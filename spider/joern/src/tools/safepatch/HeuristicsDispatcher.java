package tools.safepatch;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Predicate;
import java.util.stream.Collector;
import java.util.stream.Collectors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.eclipse.jdt.internal.codeassist.ISearchRequestor;

import ast.ASTNode;
import ast.declarations.IdentifierDecl;
import ast.expressions.AdditiveExpression;
import ast.expressions.AssignmentExpr;
import ast.expressions.CallExpression;
import ast.expressions.ConditionalExpression;
import ast.expressions.Identifier;
import ast.functionDef.Parameter;
import ast.statements.BlockStarter;
import ast.statements.CompoundStatement;
import ast.statements.Condition;
import ast.statements.DoStatement;
import ast.statements.ElseStatement;
import ast.statements.ExpressionStatement;
import ast.statements.ForStatement;
import ast.statements.IfStatement;
import ast.statements.Label;
import ast.statements.ReturnStatement;
import ast.statements.Statement;
import ast.statements.WhileStatement;
import cfg.CFG;
import ddg.DataDependenceGraph.DDG;
import ddg.DefUseCFG.DefUseCFG;
import jdk.nashorn.internal.ir.IfNode;
import tools.safepatch.conditions.ConditionDiff;
import tools.safepatch.diff.ASTDelta;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.fgdiff.ASTDiff;
import tools.safepatch.fgdiff.ASTDiff.ACTION_TYPE;
import tools.safepatch.heuristics.CodeHeuristics;
import tools.safepatch.heuristics.HeuristicsHelper;
import tools.safepatch.rw.ReadWriteTable;
import tools.safepatch.util.AffectedVariablesUtil;
import tools.safepatch.util.BasicBlock;
import tools.safepatch.util.DDGUtil;

/**
 * @author Eric Camellini
 *
 */
public class HeuristicsDispatcher {

	private static final Logger logger = LogManager.getLogger();
	
	public static enum HEURISTIC {
		KNOWN_PATCHES, MODIFIED_CONDITIONS,
		INSERTED_IFS, INIT_OR_ERROR,
		CONDITIONS, FLOW_RESTRICTIVE, REMOVED_STUFF,
		OFF_BY_ONE, INSERTED_STMT, KERNEL_SYNCH, C_SENS_FUN
		
	}

	public static enum PATCH_FEATURE {
		CHANGED_CONDITIONS_COUNT, INSERTED_IFS_COUNT,
		RESTRICTED_CONDITIONS_COUNT, WIDENED_CONDITIONS_COUNT,
		BI_IMPLICATIONS_COUNT, INSERTED_STMTS_COUNT,
	}
	 
	private Map<HEURISTIC, Boolean> enabledHeuristicsMap;
	private Map<HEURISTIC, Boolean> heuristicsResults;
	private Map<PATCH_FEATURE, Integer> patchFeatures;
	
	public HeuristicsDispatcher() {
		this.enabledHeuristicsMap = new HashMap<HEURISTIC, Boolean>();
		this.heuristicsResults = new HashMap<HEURISTIC, Boolean>();
		this.patchFeatures = new HashMap<PATCH_FEATURE, Integer>();
		
		for(HEURISTIC value : HEURISTIC.values())
			enabledHeuristicsMap.put(value, false);
	}
	
	public void enableHeuristic(HEURISTIC h){
		this.enabledHeuristicsMap.put(h, true);
	}
	
	public void disableHeuristic(HEURISTIC h){
		this.enabledHeuristicsMap.put(h, false);
	}
	
	public Boolean isEnabled(HEURISTIC h){
		return enabledHeuristicsMap.get(h);
	}
	
	public Map<HEURISTIC, Boolean> getHeuristicResults(){
		return heuristicsResults;
	}
	
	public Map<PATCH_FEATURE, Integer> getPatchFeatures(){
		return patchFeatures;
	}
	
	private CodeHeuristics codeHeuristics = new CodeHeuristics();

	/**
	 * If this field is set to true then the heuristics return false if
	 * there are some more modifications/insertions/deletes outside 
	 * the portion of code that they study.
	 * (e.g. inserted if return false is there are some changes outside the
	 * ifs that are inserted)
	 */
	private boolean exclusiveChanges = true;
	
	public void setExclusiveChanges(boolean value) {
		this.exclusiveChanges = value;
	}
	
	/**
	 * If this field is set to true then the conditions
	 * that are purely moved but not modified will be considered
	 * as changed by the changed condition heuristics.
	 * The bi-implication for these holds for sure, so they will be
	 * marked as security patches.
	 */
	private boolean considerMovedConditions = false;

	public void setConsiderMovedConditions(boolean value) {
		this.considerMovedConditions = value;
	}
	
	/**
	 * If this field is set to true then the conditions
	 * that are purely moved but not modified will be considered
	 * as changed by the changed condition heuristics.
	 * The bi-implication for these holds for sure, so they will be
	 * marked as security patches.
	 */
	private boolean bvEnabled = false;

	public void setBVEnabled(boolean value) {
		this.bvEnabled = value;
	}
	
	/*
	 * If the coarse grained diff is set, it will be
	 * used by some of the heuristics to correct some of
	 * the Gumtree possible errors.
	 */
	private List<ASTDelta> coarseGrainedDiff = null;
	
	public void setCoarseGrainedDiff(List<ASTDelta> coarseGrainedDiff){
		this.coarseGrainedDiff = coarseGrainedDiff;
	}
	
	/**
	 * This method, given a FunctionDiff, applies a series of heuristics to determine
	 * if the patch is security-related or not. 
	 */
	public boolean isSecurityPatch(FunctionDiff diff){	
		if(logger.isDebugEnabled()) logger.debug("Security patch test:");
		
		//Initializing the results map for the enabled heuristics
		heuristicsResults.clear();
		for( HEURISTIC value : HEURISTIC.values()){
			if(isEnabled(value)){
				if(logger.isDebugEnabled()) logger.debug("{} heuristic enabled.", value);
			} else {
				if(logger.isDebugEnabled()) logger.debug("{} heuristic disabled, will return always false.", value);
			}
			heuristicsResults.put(value, false);
		}
		
		//Initializing features to keep track of
		for(PATCH_FEATURE value : PATCH_FEATURE.values())
			patchFeatures.put(value, 0);
		
		
		List<ConditionDiff> modifiedConditions = null;
		if(isEnabled(HEURISTIC.CONDITIONS) || isEnabled(HEURISTIC.MODIFIED_CONDITIONS) ||
				isEnabled(HEURISTIC.FLOW_RESTRICTIVE)){
			//Type inference
			HashMap<String, String> types = new HashMap<String, String>();
			diff.typeInference();
			types.putAll(diff.getOriginalTypes());
			types.putAll(diff.getNewTypes());

			//Extracting modified conditions. The ConditionDiff object contains also the Z3 conversion and the implication information that we need
			modifiedConditions = HeuristicsHelper.getModifiedConditions(diff.getFineGrainedDiff(), types, this.considerMovedConditions, this.bvEnabled);	
		}
		
		ArrayList<IfStatement> insertedIfs = null;
		if(isEnabled(HEURISTIC.CONDITIONS) || isEnabled(HEURISTIC.INSERTED_IFS) ||
				isEnabled(HEURISTIC.FLOW_RESTRICTIVE)){
			insertedIfs = new ArrayList<IfStatement>();
			for(ASTNode n : diff.getFineGrainedDiff().getNewNodesByActionType(ACTION_TYPE.INSERT))
				if(n instanceof IfStatement) insertedIfs.add((IfStatement) n);
		}
		
		/*
		 * 1) KNOWN_PATCHES heuristic:
		 * Check for known security-related methods:
		 * if these are the only thing affected by the patch
		 * the heuristic matches.
		 */
		if(isEnabled(HEURISTIC.KNOWN_PATCHES)){
			if(logger.isDebugEnabled()) logger.debug("KNOWN_PATCHES heuristic:");
			if(knownPatchesHeuristic(diff.getFineGrainedDiff())){
				if(logger.isDebugEnabled()) logger.debug("KNOWN_PATCHES heuristic - true");
				heuristicsResults.put(HEURISTIC.KNOWN_PATCHES, true);
			} else {
				if(logger.isDebugEnabled()) logger.debug("KNOWN_PATCHES heuristic - false");
			}
		}
		
		
		/*
		 * 2) MODIFIED_CONDITIONS heuristic:
		 * True if all the modified conditions 
		 * (according to the AST diff and ignoring
		 * inserted and deleted ones) are such that there is an
		 * implication (-> <- <->) between the old and the new
		 * and match the corresponding heuristics when widened/restricted
		 * (i.e. -> or <-)
		 */
		if(isEnabled(HEURISTIC.MODIFIED_CONDITIONS)){
			if(logger.isDebugEnabled()) logger.debug("MODIFIED_CONDITIONS heuristic:");

			if(modifiedConditionsHeuritsic(modifiedConditions, diff)){
				if(logger.isDebugEnabled()) logger.debug("MODIFIED_CONDITIONS heuristic - true");
				heuristicsResults.put(HEURISTIC.MODIFIED_CONDITIONS, true);
			} else {
				if(logger.isDebugEnabled()) logger.debug("MODIFIED_CONDITIONS heuristic - false");
			}

		}

		/*
		 * 3) INSERTED_IFS heuristic:
		 * Check that the inserted ifs, if present, match the heuristic
		 * described in the matchesInsertedIfHeuristics method
		 */	
		if(isEnabled(HEURISTIC.INSERTED_IFS)){
			if(logger.isDebugEnabled()) logger.debug("INSERTED_IFS heuristic:");

			if(insertedIfsHeuristic(insertedIfs, diff.getFineGrainedDiff(), diff.getNewBasicBlocks())){
				if(logger.isDebugEnabled()) logger.debug("INSERTED_IFS heuristic - true");
				heuristicsResults.put(HEURISTIC.INSERTED_IFS, true);
			} else {
				// If the heuristic didn't match, it can be a Gumtree problem sometimes.
				// We use the coarse grained diff to correct it...
				if(this.coarseGrainedDiff != null &&
						this.coarseGrainedDiff.size() != 0 &&
						this.coarseGrainedDiff.stream().allMatch(d -> d.getType().equals(ASTDelta.DELTA_TYPE.INSERT))){
					if(logger.isDebugEnabled()) logger.debug("Using coarse grained diff for INSERTED_IFS");

					boolean allMatch = true;
					List<IfStatement >deltaInsertedIfs = new ArrayList<IfStatement>(); 
					for(ASTDelta d : this.coarseGrainedDiff){
						d.generateFineGrainedDiff();
						
						deltaInsertedIfs.clear();
						for(ASTNode n : d.getFineGrainedDiff().getNewNodesByActionType(ACTION_TYPE.INSERT))
							if(n instanceof IfStatement) deltaInsertedIfs.add((IfStatement) n);

						if(!insertedIfsHeuristic(deltaInsertedIfs, d.getFineGrainedDiff(), 
								diff.getNewBasicBlocks())) allMatch = false;
					}

					if(allMatch){
						if(logger.isDebugEnabled()) logger.debug("Coarse grained INSERTED_IFS heuristic - true");
						heuristicsResults.put(HEURISTIC.INSERTED_IFS, true);
					} else if(logger.isDebugEnabled()) logger.debug("INSERTED_IFS heuristic - false");

				} else if(logger.isDebugEnabled()) logger.debug("INSERTED_IFS heuristic - false");
			}
		}


		/*
		 * 4) INIT_OR_ERROR heuristic:
		 * If the change only affects initialization or error statements, and
		 * does not modify any other part of code, the heuristic is true.
		 * 
		 */
		if(isEnabled(HEURISTIC.INIT_OR_ERROR)){
			if(logger.isDebugEnabled()) logger.debug("INIT_OR_ERROR heuristic:");
			if(initOrErrorStatementsOnlyHeuristic(diff)){
				heuristicsResults.put(HEURISTIC.INIT_OR_ERROR, true);
				if(logger.isDebugEnabled()) logger.debug("INIT_OR_ERROR heuristic - true");
			} else {
				if(logger.isDebugEnabled()) logger.debug("INIT_OR_ERROR heuristic - false");
			}
		}
		
		/*
		 * 5) CONDITION checks
		 * This heuristic matches the MODIFIED_CONDITIONS and the INSERTED_IFS
		 * heuristics together, but allows the patch to affect also
		 * statements outside the conditions and the inserted ifs if and
		 * only if their definitions cannot (recursively) reach the variables
		 * used inside these inserted/modified conditions.
		 */
		if(isEnabled(HEURISTIC.CONDITIONS)){
			if(logger.isDebugEnabled()) logger.debug("CONDITIONS heuristic:");
			if(conditionsHeuristic(diff, modifiedConditions, insertedIfs, diff.getOriginalGraphs().getDefUseCfg(),
					diff.getNewGraphs().getDefUseCfg(), diff.getOriginalGraphs().getDdg(),
					diff.getNewGraphs().getDdg(), diff.getNewBasicBlocks())){
				if(logger.isDebugEnabled()) logger.debug("CONDITION heuristic - true");
				heuristicsResults.put(HEURISTIC.CONDITIONS, true);
			} else {
				if(logger.isDebugEnabled()) logger.debug("CONDITIONS heuristic - false");
			}
		}
		
		
		/*
		 * 6) FLOW RESTRICTIVE patches identification
		 * This heuristic implements the approach described in the paper
		 */
		if(isEnabled(HEURISTIC.FLOW_RESTRICTIVE)){
			if(logger.isDebugEnabled()) logger.debug("FLOW RESTRICTIVE heuristic:");
			if(flowRestrictive(diff, modifiedConditions, insertedIfs,
					diff.getNewGraphs().getDefUseCfg(), 
					diff.getNewGraphs().getDdg(),
					diff.getOriginalBasicBlocks(),
					diff.getNewBasicBlocks(),
					this.coarseGrainedDiff)){
				if(logger.isDebugEnabled()) logger.debug("FLOW RESTRICTIVE heuristic - true");
				heuristicsResults.put(HEURISTIC.FLOW_RESTRICTIVE, true);
			} else {
				if(logger.isDebugEnabled()) logger.debug("FLOW RESTRICTIVE heuristic - false");
			}
		}
		
		/*
		 * AFTER APPLYING THE HEURISTIC WE CHECK THEIR RESULT AND OUTPUT THE FINAL VERDICT
		 */
		
		
		// 1) If KNOWN_PATCHES is true we return true
		if(isEnabled(HEURISTIC.KNOWN_PATCHES))
			if(heuristicsResults.get(HEURISTIC.KNOWN_PATCHES) == true) return true;
		
		
		// 2) If MODIFIED_CONDITIONS is true we return true: 
		if(isEnabled(HEURISTIC.MODIFIED_CONDITIONS))
			if(heuristicsResults.get(HEURISTIC.MODIFIED_CONDITIONS) == true) return true;

		
		// 3) If INSERTED_IFS is true we return true
		if(isEnabled(HEURISTIC.INSERTED_IFS))
			if(heuristicsResults.get(HEURISTIC.INSERTED_IFS) == true) return true;
		
		
		// 4) If INIT_OR_ERROR is true we return true.
		if(isEnabled(HEURISTIC.INIT_OR_ERROR))
			if(heuristicsResults.get(HEURISTIC.INIT_OR_ERROR) == true) return true;

		// 5) If CONDITIONS is true we return true
		if(isEnabled(HEURISTIC.CONDITIONS))
			if(heuristicsResults.get(HEURISTIC.CONDITIONS) == true) return true;

		// 6) If FLOW_RESTRICTIVE is true we return true
		if(isEnabled(HEURISTIC.FLOW_RESTRICTIVE))
			if(heuristicsResults.get(HEURISTIC.FLOW_RESTRICTIVE) == true) return true;

		//No more heuristics, if we didn't return true let's return false
		return false;
	}

	private boolean flowRestrictive(FunctionDiff diff, List<ConditionDiff> modifiedConditions,
			List<IfStatement> insertedIfs, DefUseCFG newDefUseCFG, 
			DDG newDdg, List<BasicBlock> originalBasicBlocks, 
			List<BasicBlock> newBasicBlocks, List<ASTDelta> coarseGrainedDiff){

		try{
			// We first collect all the single statements affected that are not labels, block starters
			// or compound statements, all the conditions and all the parameters
			Set<ASTNode> newAffectedStatements = diff.getFineGrainedDiff()
					.getNewAffectedNodes(Statement.class).stream().filter(stmt -> (
							!(stmt instanceof CompoundStatement) 
							&& !(stmt instanceof BlockStarter)
							&& !(stmt instanceof Label)
							// This last part is to filter out the nodes wrongly considered
							// as affected by Gumtree
							&& ( (coarseGrainedDiff == null) || 
									(coarseGrainedDiff != null && 
									coarseGrainedDiff.stream()
									.anyMatch(d -> (d.getNewStatements() != null &&
									d.getNewStatements() .stream().anyMatch(s -> s.getNodes()
											.contains(stmt))))))
							)).collect(Collectors.toSet());

			Set<ASTNode> newAffectedParameters = new HashSet<ASTNode>(diff.getFineGrainedDiff().getNewAffectedNodes(Parameter.class));
			Set<ASTNode> newAffectedConditions = new HashSet<ASTNode>(diff.getFineGrainedDiff().getNewAffectedNodes(Condition.class));

			if(logger.isDebugEnabled()) logger.debug("Affected statements: {}", newAffectedStatements);
			if(logger.isDebugEnabled()) logger.debug("Affected conditons: {}", newAffectedConditions);
			if(logger.isDebugEnabled()) logger.debug("Affected parameters: {}", newAffectedParameters);
			
			// In this case it means there is only removed stuff, so we check removed statements
			// and we match the patch only if they are all in error basic blocks.
			// Most of the Gumtree/preprocessing related errors fall into this check also.
			if(insertedIfs.size() == 0 && modifiedConditions.size() == 0
					&& newAffectedStatements.size() == 0 && newAffectedParameters.size() == 0 &&
					newAffectedConditions.size() == 0){
				
				// We check only for insert because moves and updates are already excluded by the check for only deletes
				if(diff.getFineGrainedDiff().containsOnlyActionsOfType(diff.getOriginalFunction(), ACTION_TYPE.DELETE) &&
						!diff.getFineGrainedDiff().containsSomeActionsOfType(diff.getNewFunction(), ACTION_TYPE.INSERT)) {

					List<ASTNode> deletedStatements = diff.getFineGrainedDiff()
							.getOriginalNodesByActionType(ACTION_TYPE.DELETE)
							.stream().filter(n -> n instanceof Statement)
							.collect(Collectors.toList());
					
					if(logger.isDebugEnabled()) logger.debug("Studying deleted statements {}", deletedStatements);
					// We filter out else and compound statements because they cannot be inside
					// basic blocks.
					if(deletedStatements.size() != 0 && 
							deletedStatements.stream().allMatch(s -> s instanceof CompoundStatement ||
									s instanceof ElseStatement ||
									codeHeuristics.isError(findBasicBlock((Statement) s, 
											originalBasicBlocks).getBlockStatements()))) {
						heuristicsResults.put(HEURISTIC.REMOVED_STUFF, true);
						if(SafepatchMain.systemOuts) System.out.println("flow_restrictive: true: deleted stmt is compound, else or error handling block");
						return true;
					}
					else {
						if(SafepatchMain.systemOuts) System.out.println("flow_restrictive: false: no deleted stmt, or deleted stmt is not compound, else or error handling block");
						return false;
					}

				} else {
					if(SafepatchMain.systemOuts) System.out.println("flow_restrictive: false: delete is not only or patch contains insert");
					return false;
				}
			}
			
			// NOT REALLY NEEDED, output-only changes are functionality-preserving
			// If there are no affected conditions, affected parameters or inserted ifs
			// and the only change are outputOnly statements we return false.
			// This is to avoid matching useless output corrections: they are safe patches
			// but we don't need them.
//			if(codeHeuristics.outputOnly(newAffectedStatements) &&
//					newAffectedParameters.size() == 0 && insertedIfs.size() == 0 
//					&& modifiedConditions.size() ==0 &&  newAffectedConditions.size() == 0) {
//				if(logger.isDebugEnabled()) logger.debug("Output only.");
//				return false;
//			}


			// This is an heuristic to match those patches where the only thing that happens
			// is the insertion of a -1 somewhere: we found them to be all safe
			if(codeHeuristics.onlyKnownInsertionsPatterns(diff.getFineGrainedDiff())){
				if(logger.isDebugEnabled()) logger.debug("Only known insertion patterns.");
				heuristicsResults.put(HEURISTIC.OFF_BY_ONE, true);
				if(SafepatchMain.systemOuts) System.out.println("flow_restrictive: true: off_by_one is true");
				return true;
			}


			// We disable the exclusivity, so that the inserted ifs and modified conditions match the 
			// heuristics even if they are not the only modification in the patch.
			// The fact that changes outside these inserted/modified conditions 
			// should not affect the flow will be handled next
			boolean savedVal = this.exclusiveChanges;
			this.exclusiveChanges = false;

			// From the new affected statements we ignore the ones that are init or part of error basic blocks
			// or moved under an inserted if and keep only all the others.
			Set<ASTNode> flowRelevantAffectedStatements = newAffectedStatements.stream().filter(stmt -> (
					!(codeHeuristics.isInit((Statement) stmt) ||
							codeHeuristics.isError(findBasicBlock((Statement) stmt, newBasicBlocks).getBlockStatements()) ||
							(stmt.getNodes().stream().allMatch(n -> diff.getFineGrainedDiff().isMoved(n)) &&
									insertedIfs.stream().anyMatch(i -> i.getChild(1).getNodes().contains(stmt) ||
											(i.getElseNode() != null && i.getElseNode().getNodes().contains(stmt)))))))
					.collect(Collectors.toSet());

			if(logger.isDebugEnabled()) logger.debug("Flow relevant affected stmts: {}", flowRelevantAffectedStatements);

			// All these flow relevant affected statements should not affect any condition in the new function
			Set<ASTNode> newConditions = diff.getNewFunction().getNodes()
					.stream().filter(n -> n instanceof Condition).collect(Collectors.toSet());

			for(ASTNode c : newConditions){
				// We extract all the statements that define symbols that can reach the condition 
				Set<ASTNode> reachingDefs = DDGUtil
						.getAffectingDefs(c, newDdg, newDefUseCFG, true);
				// If one of the defs that can reach the condition is one of the flow relevant
				// statements or a modified parameter or a modified condition (a def can appear
				// inside conditions) we return false
				if(reachingDefs.stream().anyMatch(def -> (
						flowRelevantAffectedStatements.stream().anyMatch(s -> s.equals(def)) ||
						newAffectedParameters.stream().anyMatch(p -> p.equals(def)) ||
						newAffectedConditions.stream().anyMatch(cond -> cond.equals(def))
						))){
					if(logger.isDebugEnabled()) logger.debug("Condition affected by an affected def: {}", c);
					if(logger.isDebugEnabled()) logger.debug("Reaching defs: {}", reachingDefs);
					if(SafepatchMain.systemOuts) System.out.println("flow_restrictive: false: condition affected by an affected def");
					return false;
				}
			}
			if(logger.isDebugEnabled()) logger.debug("No conditions are affected by the flow relevant statements defs.");
			if (flowRelevantAffectedStatements.size() > 0){
				heuristicsResults.put(HEURISTIC.INSERTED_STMT, true);
				patchFeatures.put(PATCH_FEATURE.INSERTED_STMTS_COUNT, flowRelevantAffectedStatements.size());
			}

			// If there are some inserted ifs, we check that they match the inserted ifs heuristic
			if(logger.isDebugEnabled()) logger.debug("Inserted ifs: {}", insertedIfs);
			if(insertedIfs.size() != 0 &&
					!insertedIfsHeuristic(insertedIfs,
							diff.getFineGrainedDiff(), newBasicBlocks)) {
				if(SafepatchMain.systemOuts) System.out.println("flow_restrictive: false: inserted_ifs is false");
				return false;
			}

			if(logger.isDebugEnabled()) logger.debug("Inserted ifs are ok or not present.");
			if (insertedIfs.size() != 0)
				heuristicsResults.put(HEURISTIC.INSERTED_IFS, true);

			// If there are some modified conditions, we check that they match the modified conditions heuristic
			// We ignore the cases were only inline conditional expressions are affected and all these are
			// part of non flow relevant statements 
			if(logger.isDebugEnabled()) logger.debug("Modified conditions: {}", modifiedConditions);
			if(modifiedConditions.size() != 0 &&
					!modifiedConditions.stream().allMatch(c -> (
							newAffectedStatements.stream().anyMatch(s -> s.getNodes().contains(c.getNewCondition())) && 
							(flowRelevantAffectedStatements.size() == 0 || flowRelevantAffectedStatements.stream()
							.noneMatch(s -> s.getNodes().contains(c.getNewCondition())))
							)) &&
					!modifiedConditionsHeuritsic(modifiedConditions, diff)) {
				if(SafepatchMain.systemOuts) System.out.println("flow_restrictive: false: modified_conditions is false");
				return false;
			}
			if(logger.isDebugEnabled()) logger.debug("Modified conditions are ok or not present.");
			if (modifiedConditions.size() != 0)
				heuristicsResults.put(HEURISTIC.MODIFIED_CONDITIONS, true);

			// If there are no modified conditions and no inserted ifs, we want the change to be only
			// init or error. We don't check for affected function params because these are for sure only init
			if(modifiedConditions.size() == 0 && insertedIfs.size() == 0
					&& (flowRelevantAffectedStatements.size() != 0 ||
					(newAffectedConditions.size() != 0 && !newAffectedConditions.stream().allMatch(c ->
							newAffectedStatements.stream().anyMatch(s -> s.getNodes().contains(c)) && 
							(flowRelevantAffectedStatements.size() == 0 || flowRelevantAffectedStatements.stream()
							.noneMatch(s -> s.getNodes().contains(c))))))){
				if(logger.isDebugEnabled()) logger.debug("No inserted ifs or modified conditions but some statements are not init or error.");
				if(SafepatchMain.systemOuts) System.out.println("flow_restrictive: false: no modified_conditions and no inserted_ifs but some statements are not init or error");
				return false;
			}
			//heuristicsResults.put(HEURISTIC.INIT_OR_ERROR, true);
			this.exclusiveChanges = savedVal;
			if(SafepatchMain.systemOuts) System.out.println("flow_restrictive: true: matches all heuristics");
			return true;
			
		} catch (Exception e) {
			System.err.println("Error: catched " + e + " in flow heuristic.");
			if(SafepatchMain.systemOuts) System.out.println("flow_restrictive: false: terrible error");
			return false;
		}

	}
	
	private boolean conditionsHeuristic(FunctionDiff diff, List<ConditionDiff> modifiedConditions,
			List<IfStatement> insertedIfs, DefUseCFG originalDefUseCFG, DefUseCFG newDefUseCFG, 
			DDG originalDdg, DDG newDdg, List<BasicBlock> newBasicBlocks) {
		
		// 1) We check that there are some modified/inserted conditions
		if(insertedIfs.size() == 0 && modifiedConditions.size() == 0) return false;
		
		
		// We disable the exclusivity, so that the inserted ifs and modified conditions match the 
		// heuristics even if they are not the only modification in the patch.
		// The fact that changes outside these inserted/modified conditions 
		// should not affect them will be handled calling allConditionsUnaffected
		boolean savedVal = this.exclusiveChanges;
		this.exclusiveChanges = false;
		
		// 2) If there are some inserted ifs, we check that they match the inserted ifs heuristis
		// and that the patch doesn't affect variables used by the new conditions
		if(insertedIfs.size() != 0){
			ArrayList<ASTNode> insertedConditions = new ArrayList<ASTNode>();
			insertedIfs.stream().forEach(i -> insertedConditions.add(i.getCondition()));
			List<ASTNode> originalAffectedNodes = diff.getFineGrainedDiff().getOriginalNodesByAnyActionType();
			if(!allConditionsUnaffectedHeuristic(
					null, insertedConditions,
					diff, 
					null, newDefUseCFG, 
					null, newDdg) ||
					!insertedIfsHeuristic(insertedIfs, diff.getFineGrainedDiff(), newBasicBlocks) ||
					insertedConditions.stream()
					.anyMatch(c -> DDGUtil.getAffectingDefs(c, newDdg, newDefUseCFG, true).stream()
							.anyMatch(def -> originalAffectedNodes.stream()
									.anyMatch(n -> diff.getFineGrainedDiff()
											.getMappedSrcNode(def).getNodes().contains(n))))) return false;
		}

		// 3) If there are some modified conditions, we check that they match the modified conditions heuristic
		// and that the patch doesn't affect variables used by these modified conditions
		if(modifiedConditions.size() != 0){
			ArrayList<ASTNode> originalConditions = new ArrayList<ASTNode>();
			ArrayList<ASTNode> newConditions = new ArrayList<ASTNode>();
			modifiedConditions.stream().forEach(c -> {
				originalConditions.add(c.getOriginalCondition());
				newConditions.add(c.getNewCondition());
			});
			
			if(!allConditionsUnaffectedHeuristic(
					originalConditions, newConditions,
					diff, 
					originalDefUseCFG, newDefUseCFG, 
					newDdg, newDdg) ||
					!modifiedConditionsHeuritsic(modifiedConditions, diff)) return false;
		}

		this.exclusiveChanges = savedVal;
		return true;
	}

	private boolean modifiedConditionsHeuritsic(List<ConditionDiff> modifiedConditions,
			FunctionDiff diff) {
	
		if(modifiedConditions.size() == 0){
			if(logger.isDebugEnabled()) logger.debug("No modified conditions.");
			if(SafepatchMain.systemOuts) System.out.println("modified_conditions: false: no modified conditions");
			return false;
		}
		
		//Extracting the ones that imply each other
		List<ConditionDiff> restrictedConditions = HeuristicsHelper.getNewImpliesOriginal(modifiedConditions);
		List<ConditionDiff> widenedConditions = HeuristicsHelper.getOriginalImpliesNew(modifiedConditions);
		List<ConditionDiff> biImplications = HeuristicsHelper.getBiImplications(modifiedConditions);

		patchFeatures.put(PATCH_FEATURE.CHANGED_CONDITIONS_COUNT, modifiedConditions.size());
		patchFeatures.put(PATCH_FEATURE.RESTRICTED_CONDITIONS_COUNT, restrictedConditions.size());
		patchFeatures.put(PATCH_FEATURE.WIDENED_CONDITIONS_COUNT, widenedConditions.size());
		patchFeatures.put(PATCH_FEATURE.BI_IMPLICATIONS_COUNT, biImplications.size());

		
		if(logger.isDebugEnabled()) logger.debug("Changed cond: {}", modifiedConditions);
		if(logger.isDebugEnabled()) logger.debug("<-: {}", restrictedConditions);
		if(logger.isDebugEnabled()) logger.debug("->: {}", widenedConditions);
		if(logger.isDebugEnabled()) logger.debug("<->: {}", biImplications);

		// First, we check that no modified conditions are such that they don't imply each other
		if(modifiedConditions.size() != restrictedConditions.size() + widenedConditions.size() + biImplications.size()){
			if(logger.isDebugEnabled()) logger.debug("There are some modified conditions that don't imply each other.");
			if(SafepatchMain.systemOuts) System.out.println("modified_conditions: false: modified conditions don't imply each other");
			return false;
		}
		
		//Now we check that these conditions are the only thing modified
		if(exclusiveChanges){
			for(ASTNode n : diff.getFineGrainedDiff().getOriginalNodesByAnyActionType()){
				if(!modifiedConditions.stream().anyMatch(cDiff -> cDiff.getOriginalCondition().getNodes().contains(n))) {
					if(SafepatchMain.systemOuts) System.out.println("modified_conditions: false: multiple modified conditions");
					return false;
				}
			}
			
			for(ASTNode n : diff.getFineGrainedDiff().getNewNodesByAnyActionType()){
				if(!modifiedConditions.stream().anyMatch(cDiff -> cDiff.getNewCondition().getNodes().contains(n))) {
					if(SafepatchMain.systemOuts) System.out.println("modified_conditions: false: multiple modified conditions");
					return false;
				}
			}
			if(logger.isDebugEnabled()) logger.debug("Modified conditions are the only change.");
		}
		
		if (widenedConditions.size()==0 && restrictedConditions.size()==0){
			if(logger.isDebugEnabled()) logger.debug("No widened or restricted conditions.");
			if(SafepatchMain.systemOuts) System.out.println("modified_conditions: true: no widened or restricted conditions, ignoring biImplications: "+Integer.toString(biImplications.size()));
			return true;
		}
		// We then check that all widened conditions lead to an error and all
		// the restricted ones to a non-error, ignoring the biimplications
		if(widenedConditions.stream().allMatch(cond -> matchesWidenedConditionHeuristic(cond, diff)) &&
				restrictedConditions.stream().allMatch(cond -> matchesRestrictedConditionHeuristic(cond, diff))) {
			if(SafepatchMain.systemOuts) System.out.println("modified_conditions: true: matches all heuristics");
			return true;
		}
		else {
			if(SafepatchMain.systemOuts) System.out.println("modified_conditions: false: widened or restricted conditions don't match heuristics");
			return false;
		}
	}

	private boolean matchesRestrictedConditionHeuristic(ConditionDiff cond, FunctionDiff diff) {
		Condition c = cond.getNewCondition();
		
		//If it's an if condition
		if(c.getParent() instanceof IfStatement){
			// If an If condition is restricted then it means that the code
			// enters in the true code block less times than before, so we want it
			// to be a non-error block
			ASTNode then = c.getParent().getChild(1);
			if(!blockErrorHeuristic((Statement) then, diff.getNewBasicBlocks())) {
				if(SafepatchMain.systemOuts) System.out.println("restricted_conditions: true: not error handling block");
				return true;
			}
		}
		
		if(c.getParent() instanceof ForStatement ||
				c.getParent() instanceof WhileStatement ||
				c.getParent() instanceof DoStatement) {
					if(SafepatchMain.systemOuts) System.out.println("restricted_conditions: true: not if stmt");
					return true;
				}

		if(SafepatchMain.systemOuts) System.out.println("restricted_conditions: false: error handling block");
		return false;
	}

	private boolean matchesWidenedConditionHeuristic(ConditionDiff cond, FunctionDiff diff) {
		Condition c = cond.getNewCondition();
		
		//If it's an if condition
		if(c.getParent() instanceof IfStatement){
			// If an If condition is widened then it means that the code
			// enters in the true code block more times than before, so we want it
			// to be an error block
			ASTNode then = c.getParent().getChild(1);
			if(blockErrorHeuristic((Statement) then, diff.getNewBasicBlocks())) {
				if(SafepatchMain.systemOuts) System.out.println("widened_conditions: true: error handling block");
				return true;
			}
		}
		
		if(SafepatchMain.systemOuts) System.out.println("widened_conditions: false: not error handling block or not if stmt");
		return false;
	}

	private boolean insertedIfsHeuristic(List<IfStatement> insertedIfs, ASTDiff fineGrainedDiff, List<BasicBlock> newBasicBlocks) {
		if(logger.isDebugEnabled()) logger.debug("ifs: {}", insertedIfs);
		
		patchFeatures.put(PATCH_FEATURE.INSERTED_IFS_COUNT, insertedIfs.size());
		
		if(insertedIfs.size() == 0){
			if(logger.isDebugEnabled()) logger.debug("There are no inserted ifs");
			if(SafepatchMain.systemOuts) System.out.println("inserted_ifs: false: no inserted ifs");
			return false;
		}
		
		if(exclusiveChanges){
			// We collect original updated/deleted nodes not considering labels or their child identifier
			Collection<ASTNode> originalDeletedNodes = fineGrainedDiff.getOriginalNodesByActionType(ACTION_TYPE.DELETE)
					.stream().filter(n -> (!(n instanceof Label) && !(n.getParent() instanceof Label))).collect(Collectors.toList());
			Collection<ASTNode> originalUpdatedNodes = fineGrainedDiff.getOriginalNodesByActionType(ACTION_TYPE.UPDATE)
					.stream().filter(n -> (!(n instanceof Label) && !(n.getParent() instanceof Label))).collect(Collectors.toList());;
			
			/*
			 * We check that the only modifications are the inserted ifs themselves
			 * and the stuff that is inside them, and that there is no deleted or 
			 * updated stuff. That's because we want the nodes of the if to be either
			 * completely new or completely moved but unmodified.
			 */
			if(originalDeletedNodes.size() != 0 || originalUpdatedNodes.size() != 0){
				if(SafepatchMain.systemOuts) System.out.println("inserted_ifs: false: complex modification");
				return false;
			}
			
			// We skip labels also here
			for(ASTNode n : fineGrainedDiff.getNewNodesByAnyActionType()
					.stream().filter(n -> (!(n instanceof Label) && !(n.getParent() instanceof Label))).collect(Collectors.toList())){
				if(!insertedIfs.stream().anyMatch(ifStmt -> ifStmt.getNodes().contains(n))){
					if(SafepatchMain.systemOuts) System.out.println("inserted_ifs: false: labels");
					return false;
				}
			}
		}
		
		if(insertedIfs.size() != 0 &&
				insertedIfs.stream().allMatch(
						ifStmt -> (matchesInsertedIfHeuristics(ifStmt, fineGrainedDiff, newBasicBlocks)))){
			if(logger.isDebugEnabled()) logger.debug("All ifs matches.");
			return true;
		} else return false;
	}
	
	/*
	 * Inserted if heuristics:
	 * 
	 * If there is no else, the then compound must be one of the following: 
	 * 	- an error basic block;
	 *  - a piece of code that was already there (only moved code);
	 *  - an inserted if that matches this same heuristics.
	 * 
	 * If the else is present then one of the following must hold:
	 * 	- the then part is an error basic block and the else is either only moved code
	 * 	  	or another if that matches this heuristic;
	 *  - same as above, but inverting else and then;
	 *  - both else and then are inserted ifs that match this same heuristic.
	 *  
	 * and the else an error one, or vice versa, or both of them only moved;
	 * 
	 */
	private boolean matchesInsertedIfHeuristics(IfStatement ifStmt, ASTDiff fineGrainedDiff, List<BasicBlock> newBasicBlocks) {
		ASTNode thenNode = ifStmt.getChild(1);
		if(ifStmt.getElseNode() == null) {
			boolean res1=blockErrorHeuristic((Statement) thenNode, newBasicBlocks);
			boolean res2=blockMovedHeuristic((Statement) thenNode, fineGrainedDiff);
			boolean res3=blockIsInsertedIf((Statement) thenNode, fineGrainedDiff, newBasicBlocks);
			boolean res4=singleInitStatement((Statement) thenNode, fineGrainedDiff);
			if (res1)
				if(SafepatchMain.systemOuts) System.out.println("inserted_if: true: no else and error handling block");
			if (res2)
				if(SafepatchMain.systemOuts) System.out.println("inserted_if: true: no else and block simply moved");
			if (res3)
				if(SafepatchMain.systemOuts) System.out.println("inserted_if: true: no else and then is inserted_if and matches heuristics");
			if (res4)
				if(SafepatchMain.systemOuts) System.out.println("inserted_if: true: no else and single init statement");
			return res1 || res2 || res3 || res4;
		} else {
			ASTNode elseNode = ifStmt.getElseNode().getChild(0);
			boolean res1=(blockErrorHeuristic((Statement) thenNode, newBasicBlocks) &&
					(blockMovedHeuristic((Statement) elseNode, fineGrainedDiff) ||
					blockIsInsertedIf((Statement) elseNode, fineGrainedDiff, newBasicBlocks)));
			boolean res2=(blockErrorHeuristic((Statement) elseNode, newBasicBlocks) &&
					(blockMovedHeuristic((Statement) thenNode, fineGrainedDiff) ||
					blockIsInsertedIf((Statement) thenNode, fineGrainedDiff, newBasicBlocks)));
			boolean res3=(blockIsInsertedIf((Statement) thenNode, fineGrainedDiff, newBasicBlocks) &&
					blockIsInsertedIf((Statement) elseNode, fineGrainedDiff, newBasicBlocks));
			boolean res4=(singleInitStatement((Statement) thenNode, fineGrainedDiff) &&
					blockIsInsertedIf((Statement) elseNode, fineGrainedDiff, newBasicBlocks));
			if (res1)
				if(SafepatchMain.systemOuts) System.out.println("inserted_if: true: else exists and then is error handling blocks");
			if (res2)
				if(SafepatchMain.systemOuts) System.out.println("inserted_if: true: else exists and else is error handling block");
			if (res3)
				if(SafepatchMain.systemOuts) System.out.println("inserted_if: true: else exists and both are inserted ifs");
			if (res4)
				if(SafepatchMain.systemOuts) System.out.println("inserted_if: true: else exists and then is single init statement");
			return res1 || res2 || res3 || res4;
		}
	}

	private boolean blockIsInsertedIf(Statement body, ASTDiff fineGrainedDiff, List<BasicBlock> newBasicBlocks){
		Statement maybeIfStatement = body;
		if(body instanceof CompoundStatement && body.getChildCount() == 1)
			maybeIfStatement = (Statement) body.getChild(0);
		
		return maybeIfStatement instanceof IfStatement &&
				fineGrainedDiff.isInserted(maybeIfStatement) &&
				matchesInsertedIfHeuristics((IfStatement) maybeIfStatement, fineGrainedDiff, newBasicBlocks);
	}
	

	private boolean blockErrorHeuristic(Statement body, List<BasicBlock> basicBlocksList) {
		
		if(logger.isDebugEnabled()) logger.debug("Checking blockErrorHeuristic for {}", body);
		//if(logger.isDebugEnabled()) logger.debug("Parent: {}", body.getParent());
		
		List<ASTNode> stmtList;
		if(body instanceof CompoundStatement) stmtList = ((CompoundStatement) body).getStatements();
		else stmtList = singleASTNodeList(body);
		
		if(logger.isDebugEnabled()) logger.debug(stmtList);
		//First, check if it is a basic block. If yes, apply  error code heuristics
		if(!basicBlocksList.stream().anyMatch(block -> block.getBlockStatements().equals(stmtList))){
			if(logger.isDebugEnabled()) logger.debug("Not a basic block.");
			return false;
		}
		else return codeHeuristics.isError(stmtList);
	}

	private boolean blockMovedHeuristic(Statement body, ASTDiff diff) {
		if(body instanceof CompoundStatement){
			return body.getChildCount() != 0 &&
					body.getChildren().stream().allMatch(
					child -> child.getNodes().stream().allMatch(stmt -> diff.isMoved(stmt)));
		} else return body.getNodes().stream().allMatch(stmt -> diff.isMoved(stmt));
	}
	
	private boolean singleInitStatement(Statement body, ASTDiff diff){
		if(body instanceof CompoundStatement) return (body.getChildCount() == 1 &&
				diff.isInserted(body.getChild(0)) &&
				(body.getChild(0) instanceof Statement &&
						codeHeuristics.isInit((Statement) body.getChild(0))));
		else return (diff.isInserted(body) && 
				(body instanceof Statement &&
						codeHeuristics.isInit((Statement) body)));
	}
	
	private List<ASTNode> singleASTNodeList(ASTNode node){
		ArrayList<ASTNode> l = new ArrayList<>();
		l.add(node);
		return l;
	}
	
	private boolean initOrErrorStatementsOnlyHeuristic(FunctionDiff diff) {
		//We extract the single statements affected
		List<ASTNode> originalAffectedStatements = diff.getFineGrainedDiff().getOriginalAffectedNodes(Statement.class);
		List<ASTNode> newAffectedStatements = diff.getFineGrainedDiff().getNewAffectedNodes(Statement.class);
		Set<ASTNode> affectedStatements = new HashSet<ASTNode>();
		//We ignore labels, compounds and block starters: only single line statements.
		originalAffectedStatements.forEach(stmt -> {
			if(!(stmt instanceof CompoundStatement) 
					&& !(stmt instanceof BlockStarter)
					&& !(stmt instanceof Label)) affectedStatements.add((Statement) stmt);
		});
		newAffectedStatements.forEach(stmt -> {
			if(!(stmt instanceof CompoundStatement) 
					&& !(stmt instanceof BlockStarter)
					&& !(stmt instanceof Label)) affectedStatements.add((Statement) stmt);
		});
		
		if(logger.isDebugEnabled()) logger.debug("Affected statements: {}", affectedStatements);
		
		if(affectedStatements.size() == 0) return false;
		
		if(codeHeuristics.outputOnly(affectedStatements)) return false;
		if(codeHeuristics.onlyKnownInsertionsPatterns(diff.getFineGrainedDiff())) return true;
		
		// Let's check that these statements are the only things modified
		if(exclusiveChanges){
			
			//We use affected statements because it contains the statements filtered to remove compounds block starters,
			//so that if there are block starters affected we return false
			
			for(ASTNode n : diff.getFineGrainedDiff().getOriginalNodesByAnyActionType()
					.stream().filter(n -> (!(n instanceof Label) && 
							!(n.getParent() instanceof Label))).collect(Collectors.toList())){
				if(!affectedStatements.stream().anyMatch(stmt -> stmt.getNodes().contains(n)))
					return false;
			}
			
			// If compounds or else are fully inserted we can take them into account, so we don't filter them
			for(ASTNode n : diff.getFineGrainedDiff().getNewNodesByAnyActionType()
					.stream().filter(n -> (!(n instanceof Label) && !(n.getParent() instanceof Label))).collect(Collectors.toList())){
				if(n instanceof CompoundStatement || n instanceof ElseStatement){ 
					if(!diff.getFineGrainedDiff().isInserted(n)) return false;
					else continue;
				}
				if(!affectedStatements.stream().anyMatch(stmt -> stmt.getNodes().contains(n)))
					return false;
			}
		}
			
		boolean onlyDeletes = codeHeuristics.onlyKnownDeletesPatterns(diff.getFineGrainedDiff());
		
		//If all the affected statements are init statements or part of error
		//basic blocks we return true
		for(ASTNode stmt : affectedStatements){
			
			// If it is an init statement and the commit is not only part 
			// of a known set of delete patterns we go on
			if(codeHeuristics.isInit((Statement) stmt) && !onlyDeletes)
				continue;
			
			// Otherwise we check if it is inside an error basic block
			//Let's locate the basic block
			BasicBlock b = findBasicBlock((Statement) stmt, diff.getOriginalBasicBlocks());
			if(b == null) b = findBasicBlock((Statement) stmt, diff.getNewBasicBlocks());
			
			// If it doesn't match any of the init/error heuristics we return false
			if(!(b != null && codeHeuristics.isError(b.getBlockStatements()))) return false;
		}
		
		return true;
		
	}
	
	private BasicBlock findBasicBlock(Statement statement, List<BasicBlock> basicBlocksList){
		BasicBlock retVal = null;
		if(logger.isDebugEnabled()) logger.debug("Locating basic block for {}", statement);

		for(BasicBlock b : basicBlocksList){
			if(b.getBlockStatements().contains(statement)){
				retVal = b;
			} 
			else if(statement instanceof BlockStarter){
				Condition c = null;
				
				for(ASTNode child : statement.getChildren()) 
					if(child instanceof Condition) c = (Condition) child;
				
				if(c != null && b.getBlockStatements().contains(c))
					retVal = b;
			}
		}
		if(logger.isDebugEnabled()) logger.debug("Basic block found: {}", retVal);
		return retVal;
	}
	
	private boolean knownPatchesHeuristic(ASTDiff diff) {
		
		//Getting all the affected CallExpressions
		List<ASTNode> originalAffectedCalls = diff.getOriginalAffectedNodes(CallExpression.class);
		List<ASTNode> newAffectedCalls = diff.getNewAffectedNodes(CallExpression.class);
		
		if(originalAffectedCalls.size() == 0 && newAffectedCalls.size() == 0){
			if(logger.isDebugEnabled()) logger.debug("There are no affected calls");
			return false;
		}
		
		if(logger.isDebugEnabled()) logger.debug("Original affected calls {}", originalAffectedCalls);
		if(logger.isDebugEnabled()) logger.debug("New affected calls {}", newAffectedCalls);
		
		//We check that these are the only modifications
		if(exclusiveChanges){
			
			for(ASTNode n : diff.getOriginalNodesByAnyActionType()){
				// We skip the ExpressionSatatement nodes that are CallExpression, since we
				// already consider the CallExpression node
				if(n instanceof ExpressionStatement && n.getChild(0) instanceof CallExpression)
					continue;
				if(!originalAffectedCalls.stream().anyMatch(call -> call.getNodes().contains(n)))
					return false;
			}
			
			for(ASTNode n : diff.getNewNodesByAnyActionType()){
				// We skip the ExpressionSatatement nodes that are CallExpression, since we
				// already consider the CallExpression node
				if(n instanceof ExpressionStatement && n.getChild(0) instanceof CallExpression)
					continue;
				if(!newAffectedCalls.stream().anyMatch(call -> call.getNodes().contains(n))){
					return false;
				}
			}
		}
		
		Set<CallExpression> affectedCalls = new HashSet<CallExpression>();
		
		//We put them all together and remove the nested ones, considering only the outermost.
		//This is useful to avoid, for example, considering sizeof() instead of the external
		//call that uses it as an argument.		
		originalAffectedCalls.forEach(call -> {
			if(!call.getParents().stream().anyMatch(p -> p instanceof CallExpression))
				affectedCalls.add((CallExpression) call);
		});
		
		newAffectedCalls.forEach(call -> {
			if(!call.getParents().stream().anyMatch(p -> p instanceof CallExpression))
				affectedCalls.add((CallExpression) call);
		});
		
		if(affectedCalls.stream().allMatch(call -> knownMethodCheck(call, diff))) return true;
		else return false;
	}
	
	private static final String SCANF = "scanf";
	private static final String PRINTF = "printf";
	private static final String LOCK = "lock";
	private static final String UNLOCK = "unlock";
	
	static final String[] knownMethodsArray = new String[] { "strcpy", "strncpy", "strlcpy", "memcpy" };
	private static final List<String> knownMethods = Arrays.asList(knownMethodsArray);
	
	private boolean knownMethodCheck(CallExpression call, ASTDiff diff){
		if(logger.isDebugEnabled()) logger.debug("Known method check {} ", call);
		String callee = call.getChild(0).getEscapedCodeStr();
		if (((callee.toLowerCase().contains(SCANF) && !callee.startsWith(SCANF) && callee.startsWith("s"))||
				(callee.toLowerCase().contains(PRINTF) && callee.toLowerCase().startsWith("s")) ||
				knownMethods.stream().anyMatch(method -> callee.contains(method)))
				&& !diff.isMoved(call) && !diff.isDeleted(call) && !diff.isInserted(call) && !diff.isUpdated(call))
			heuristicsResults.put(HEURISTIC.C_SENS_FUN, true);

		if (callee.toLowerCase().endsWith("_" + LOCK) || callee.toLowerCase().startsWith(LOCK + "_") 
				|| callee.toLowerCase().contains("_" + LOCK + "_") || callee.toLowerCase().equals(LOCK)
				|| callee.toLowerCase().endsWith("_" + UNLOCK) || callee.toLowerCase().startsWith(UNLOCK + "_") 
				|| callee.toLowerCase().contains("_" + UNLOCK + "_") || callee.toLowerCase().equals(UNLOCK))
			heuristicsResults.put(HEURISTIC.KERNEL_SYNCH, true);
			
		return (((callee.toLowerCase().contains(SCANF) && !callee.startsWith(SCANF) && callee.startsWith("s"))||
				(callee.toLowerCase().contains(PRINTF) && callee.toLowerCase().startsWith("s")) ||
				knownMethods.stream().anyMatch(method -> callee.contains(method)))
				&& !diff.isMoved(call) && !diff.isDeleted(call) && !diff.isInserted(call) && !diff.isUpdated(call))
				|| callee.toLowerCase().endsWith("_" + LOCK) || callee.toLowerCase().startsWith(LOCK + "_") 
				|| callee.toLowerCase().contains("_" + LOCK + "_") || callee.toLowerCase().equals(LOCK)
				|| callee.toLowerCase().endsWith("_" + UNLOCK) || callee.toLowerCase().startsWith(UNLOCK + "_") 
				|| callee.toLowerCase().contains("_" + UNLOCK + "_") || callee.toLowerCase().equals(UNLOCK);
	}

	/**
	 * This heuristic checks if the patch affects the control flow in some way.
	 * 
	 * To do that, it finds variables used by all the conditions and extracts the
	 * definitions that (recursively) affect each one of these variables. If one or
	 * more of these definition statements are affected by the patch the heuristic
	 * will return false, true otherwise. 
	 */
	private boolean allConditionsUnaffectedHeuristic(List<ASTNode> originalConditions, List<ASTNode> newConditions,
			FunctionDiff diff, DefUseCFG originalDefUseCFG, DefUseCFG newDefUseCFG, 
			DDG originalDdg, DDG newDdg) {
		
		if(originalConditions != null){
			List<ASTNode> originalAffectedNodes = diff.getFineGrainedDiff().getOriginalNodesByAnyActionType();
			if(logger.isDebugEnabled()) logger.debug("Original affected nodes {} ", originalAffectedNodes);
			// For every original condition we extract (recursively) all the defs
			// that affect variables that the condition uses.
			// If one of these def statements is affected by the patch we return false.
			for(ASTNode condition : originalConditions){
				Set<ASTNode> reachingDefs = DDGUtil.getAffectingDefs(condition, originalDdg, originalDefUseCFG, true);
				if(logger.isDebugEnabled()) logger.debug("Original condition {} ", condition);
				if(logger.isDebugEnabled()) logger.debug("reaching defs {} ", reachingDefs);
				if(reachingDefs.stream()
						.anyMatch(def -> originalAffectedNodes.stream()
								.anyMatch(n -> def.getNodes().contains(n)))){
					if(logger.isDebugEnabled()) logger.debug("Condition is affected.");
					return false;
				}
			}
		}
		
		// We do the same for the new conditions
		if(newConditions != null){
			List<ASTNode> newAffectedNodes = diff.getFineGrainedDiff().getNewNodesByAnyActionType();
			if(logger.isDebugEnabled()) logger.debug("New affected nodes {} ", newAffectedNodes);
			for(ASTNode condition : newConditions){
				Set<ASTNode> reachingDefs = DDGUtil.getAffectingDefs(condition, newDdg, newDefUseCFG, true);
				if(logger.isDebugEnabled()) logger.debug("New condition {} ", condition);
				if(logger.isDebugEnabled()) logger.debug("reaching defs {} ", reachingDefs);
				if(reachingDefs.stream()
						.anyMatch(def -> newAffectedNodes.stream()
								.anyMatch(n -> def.getNodes().contains(n)))){
					if(logger.isDebugEnabled()) logger.debug("Condition is affected.");
					return false;
				}
			}
		}
		
		if(logger.isDebugEnabled()) logger.debug("No conditions affected by the change.");
		return true;
	}
}


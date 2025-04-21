/**
 * 
 */
package tools.safepatch.iocorrespondence;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.microsoft.z3.Context;

import ddg.DataDependenceGraph.DDG;
import ddg.DefUseCFG.DefUseCFG;
import ast.ASTNode;
import ast.expressions.AssignmentExpr;
import ast.statements.Condition;
import ast.statements.ReturnStatement;
import tools.safepatch.cfgclassic.BasicBlock;
import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.fgdiff.StatementMap;
import tools.safepatch.fgdiff.StatementMap.MAP_TYPE;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import tools.safepatch.z3stuff.GlobalZ3ObjManager;
import tools.safepatch.z3stuff.exprhandlers.Z3Utils;
import tools.safepatch.flowres.FunctionMap;

/**
 * @author machiry
 *
 */
public class FlowRestrictiveChecker {
	
	private static final Logger logger = LogManager.getLogger();
	private FunctionMap targetMap = null;
	private HashMap<ASTNode, ArrayList<String>> oldFuncAffectedVars = null;
	private HashMap<ASTNode, ArrayList<String>> newFuncAffectedVars = null;
	private HashMap<ASTNode, HashSet<String>> oldFuncAffectedUseVars = new HashMap<ASTNode, HashSet<String>>();
	private HashMap<ASTNode, HashSet<String>> newFuncAffectedUseVars = new HashMap<ASTNode, HashSet<String>>();
	private HashMap<ASTNode, ASTNode> conditionsToCheck = null;
	private ArrayList<ASTNode> unModifiedConditions = new ArrayList<ASTNode>(); 
	
	// for each statement, the set of affected statements which directly influence it.
	private HashMap<ASTNode, HashSet<ASTNode>> oldstatementDependency = new HashMap<ASTNode, HashSet<ASTNode>>();
	private HashMap<ASTNode, HashSet<ASTNode>> oldcondTransDependency = null;
	private HashMap<ASTNode, HashSet<ASTNode>> newstatementDependency = new HashMap<ASTNode, HashSet<ASTNode>>();
	private HashMap<ASTNode, HashSet<ASTNode>> newcondTransDependency = null;
	
	private HashSet<StatementMap> transitivelyAffectedStatements = null;
	
	private HashSet<ASTNode> oldFuncTransitvelyAffectedStatements = null;
	private HashSet<ASTNode> newFuncTransitvelyAffectedStatements = null;
	 
	
	private HashMap<ASTNode, SATISFIABLE_FLAG> condStaisFlags = null;
	private static Context ctx = null;
	private ASTNodeToZ3Converter oldFuncConverter = null;
	private ASTNodeToZ3Converter newFuncConverter = null;
	
	public enum SATISFIABLE_FLAG {
		NEW_IMPL_OLD,
		OLD_IMPL_NEW,
		SAME,
		OTHER
	};
	
	public FlowRestrictiveChecker(FunctionMap currMap) {
		this.targetMap = currMap;
		if(ctx == null) {
			ctx = new Context();
		}
		this.computeAffectedVariables();
		if(!initZ3State()) {
			logger.error("Error occurred while trying to initialize z3 state\n");
		}
	}
	
	
	/***
	 *  Initialize z3 state with the affected variables from old function and new function.
	 * @return true if initialization is successful else false.
	 */
	private boolean initZ3State() {
		boolean retVal = true;
		try {
			GlobalZ3ObjManager.setFunctionMap(this.targetMap);
		
			oldFuncConverter = new ASTNodeToZ3Converter(this.targetMap, this.targetMap.getOldFuncGraphs(), this.oldFuncAffectedVars, this.oldFuncAffectedUseVars, true, this.ctx);
			this.targetMap.setOldFnConv(oldFuncConverter);
			
			newFuncConverter = new ASTNodeToZ3Converter(this.targetMap, this.targetMap.getNewFuncGraphs(), this.newFuncAffectedVars, this.newFuncAffectedUseVars, false, this.ctx);
			this.targetMap.setNewFnConv(newFuncConverter);
			
			oldFuncConverter.setOldFuncConverter(oldFuncConverter);
			oldFuncConverter.setNewFuncConverter(newFuncConverter);
			newFuncConverter.setOldFuncConverter(oldFuncConverter);
			newFuncConverter.setNewFuncConverter(newFuncConverter);
			this.targetMap.setZ3Ctx(this.ctx);
			
		} catch(Exception e) {
			retVal = false;
			e.printStackTrace();
		}
		return retVal;
	}
	
	public boolean isOldStmtHandled(ASTNode oldSt) {
		this.computeAffectedVariables();
		return this.oldFuncAffectedVars.keySet().contains(oldSt);
	}
	
	public boolean isNewStmtHandled(ASTNode newSt) {
		this.computeAffectedVariables();
		return this.newFuncAffectedVars.keySet().contains(newSt);
	}
	
	public boolean isNewStmtAffectsCondtion(ASTNode newSt) {
		if(this.isNewStmtHandled(newSt)) {
			return this.getConditionsToCheck().size() > 0;
		}
		return false;
	}
	
	public HashMap<ASTNode, ASTNode> getConditionsToCheck() {
		return this.conditionsToCheck;
	}
	
	public HashMap<ASTNode, SATISFIABLE_FLAG> getConCheckFlags() {
		return this.condStaisFlags;
	}
	
	public HashSet<StatementMap> getTransitivelyAffectedStatements() {
		this.computeTransitivelyAffectedStatements();
		return this.transitivelyAffectedStatements;
	}
	
	public boolean isOldNodeInErrorBB(ASTNode oldASTNode) {
		if(oldASTNode != null) {
			if(this.targetMap.getOldClassicCFG().getBB(oldASTNode).isConfigErrorHandling()) {
				return true;
			}
		}
		return false;
	}
	
	public boolean isNewNodeInErrorBB(ASTNode newASTNode) {
		if(newASTNode != null) {
			if(this.targetMap.getNewClassicCFG().getBB(newASTNode).isConfigErrorHandling()) {
				return true;
			}
		}
		return false;
	}
	
	public boolean hasDeletedConditions() {
		for(StatementMap currStMap: this.targetMap.getStatementMap()) {
			if(currStMap.getMapType() == MAP_TYPE.DELETE  && currStMap.getOriginal() instanceof Condition) {
				return true;
			}
		}
		return false;
	}
	
	public boolean computeTransitivelyAffectedStatements() {
		if(this.transitivelyAffectedStatements == null) {
			this.computeConditionTransDependency();
			this.transitivelyAffectedStatements = new HashSet<StatementMap>();
			this.oldFuncTransitvelyAffectedStatements = new HashSet<ASTNode>();
			this.newFuncTransitvelyAffectedStatements = new HashSet<ASTNode>();
			
			for(ASTNode currOldNode: this.oldstatementDependency.keySet()) {
				StatementMap currStMap = this.targetMap.getTargetStMap(currOldNode, true);
				if(currStMap.getMapType() == MAP_TYPE.UNMODIFIED && !this.isoldASTNodeAffectingStatement(currOldNode)) {
					BasicBlock currBB = this.targetMap.getOldClassicCFG().getBB(currStMap.getOriginal());
					BasicBlock newBB = this.targetMap.getNewClassicCFG().getBB(currStMap.getNew());
					if(!currBB.isConfigErrorHandling() && !newBB.isConfigErrorHandling()) {
						this.transitivelyAffectedStatements.add(currStMap);
						
						this.oldFuncTransitvelyAffectedStatements.add(currStMap.getOriginal());
						this.newFuncTransitvelyAffectedStatements.add(currStMap.getNew());
					}
					
				}
			}
			
			for(ASTNode currNewNode: this.newstatementDependency.keySet()) {
				StatementMap currStMap = this.targetMap.getTargetStMap(currNewNode, false);
				if(currStMap.getMapType() == MAP_TYPE.UNMODIFIED && !this.isnewASTNodeAffectingStatement(currNewNode)) {
					
					BasicBlock currBB = this.targetMap.getOldClassicCFG().getBB(currStMap.getOriginal());
					BasicBlock newBB = this.targetMap.getNewClassicCFG().getBB(currStMap.getNew());
					if(!currBB.isConfigErrorHandling() && !newBB.isConfigErrorHandling()) {
						this.transitivelyAffectedStatements.add(currStMap);
						
						this.oldFuncTransitvelyAffectedStatements.add(currStMap.getOriginal());
						this.newFuncTransitvelyAffectedStatements.add(currStMap.getNew());
					}
				}
			}
			
			this.oldFuncConverter.setTransAffectedVars(this.oldFuncTransitvelyAffectedStatements);
			this.newFuncConverter.setTransAffectedVars(this.newFuncTransitvelyAffectedStatements);
			
		}
		
		return this.transitivelyAffectedStatements != null;
	}
	
	/***
	 *  Check the implication between an old function and new function ast node.
	 *  
	 * @param oldASTNode Old function AST Node.
	 * @param newASTNode new function AST node.
	 * @param expectedFlag expected implication to check
	 * @return true if the requested implication is true.
	 */
	public boolean checkConditionImplication(ASTNode oldASTNode, ASTNode newASTNode, SATISFIABLE_FLAG expectedFlag) {
		boolean retVal = false;
		logger.debug("Trying to convert old condition {} to z3 obj", oldASTNode.getEscapedCodeStr());
		Object oldCondZ3Obj = oldFuncConverter.getZ3Expression(oldASTNode, null);
		logger.debug("Trying to convert new condition {} to z3 obj", newASTNode.getEscapedCodeStr());
		Object newCondZ3Obj = newFuncConverter.getZ3Expression(newASTNode, null);
		if(expectedFlag == SATISFIABLE_FLAG.OLD_IMPL_NEW) {
			if(Z3Utils.checkImplication(this.ctx, oldCondZ3Obj, newCondZ3Obj)) {
				retVal = true;
			}
		}
		if(expectedFlag == SATISFIABLE_FLAG.NEW_IMPL_OLD) {
			if(Z3Utils.checkImplication(this.ctx, newCondZ3Obj, oldCondZ3Obj)) {
				retVal = true;
			}
		}
		if(expectedFlag == SATISFIABLE_FLAG.SAME) {
			if(Z3Utils.checkEquivalence(this.ctx, oldCondZ3Obj, newCondZ3Obj)) {
				retVal = true;
			}
		}
		
		return retVal;
	}
	
	public boolean checkUpdatedStatementEquivalence(StatementMap currMap) {
		boolean retVal = true;
		if(currMap.getNew() instanceof AssignmentExpr && 
		   currMap.getOriginal() instanceof AssignmentExpr) {
			retVal = false;
			AssignmentExpr oldSt = (AssignmentExpr)currMap.getOriginal();
			
			ASTNode leftChild = oldSt.getLeft();
					
			ASTNode rightChild = oldSt.getRight();
			
			Object oldUpdatedSt = oldFuncConverter.getZ3Expression(rightChild, leftChild.getEscapedCodeStr().trim());
			
			AssignmentExpr newSt = (AssignmentExpr)currMap.getNew();
			
			leftChild = newSt.getLeft();
			
			rightChild = newSt.getRight();
			
			Object newUpdatedSt = newFuncConverter.getZ3Expression(rightChild, leftChild.getEscapedCodeStr().trim());
			// both the updated statements should be equivalent.
			if(Z3Utils.checkEquivalence(this.ctx, oldUpdatedSt, newUpdatedSt)) {
				retVal = true;
			}
			
		}
		
		// if this is a return  statement..check the return value equivalence
		if(currMap.getNew() instanceof ReturnStatement && 
			currMap.getOriginal() instanceof ReturnStatement) {
			retVal = false;
			ReturnStatement oldSt = (ReturnStatement)currMap.getOriginal();
					
			ReturnStatement newSt = (ReturnStatement)currMap.getNew();
			
			if(oldSt.getChildren() == null || newSt.getChildren() == null) {
				return oldSt.getChildren() == null && newSt.getChildren() == null;
			}
			
			if(oldSt.getChildren().isEmpty() || newSt.getChildren().isEmpty()) {
				return oldSt.getChildren().isEmpty() && newSt.getChildren().isEmpty();
			}
			
			if(oldSt.getChildren().size() == newSt.getChildren().size()) {
			
				ASTNode funcRetVal = oldSt.getChildren().get(0);
				
				Object oldUpdatedSt = oldFuncConverter.getZ3Expression(oldSt, funcRetVal.getEscapedCodeStr().trim());
						
				funcRetVal = newSt.getChildren().get(0);
						
				Object newUpdatedSt = newFuncConverter.getZ3Expression(newSt, funcRetVal.getEscapedCodeStr().trim());
				// both the updated statements should be equivalent.
				if(Z3Utils.checkEquivalence(this.ctx, oldUpdatedSt, newUpdatedSt)) {
					retVal = true;
				}
			} else {
				retVal = false;
			}
		}
		
		return retVal;
	}
	
	
	/***
	 *  Function to check if there are any affected conditions.
	 * @return true if there exists affected conditions else false.
	 */
	public boolean areConditionsAffected() {
		this.computeConditionsToCheck();
		return !this.conditionsToCheck.isEmpty();
	}
	
	/***
	 * 
	 * @return
	 */
	public boolean computeConditionsToCheck() {
		try {
			if(this.conditionsToCheck == null) {
				if(!this.computeAffectedVariables()) {
					FlowRestrictiveChecker.logger.error("Unable to get affected variables list");
				} else {
					HashMap<ASTNode, ASTNode> condsToCheck = new HashMap<ASTNode, ASTNode>();
					HashMap<ASTNode, ASTNode> allCondMap = this.targetMap.getMappedConditions();
					for(ASTNode oldCo:allCondMap.keySet()) {
						ASTNode newCond = allCondMap.get(oldCo);
						if(this.oldFuncAffectedVars.containsKey(oldCo) || this.newFuncAffectedVars.containsKey(newCond)) {
							condsToCheck.put(oldCo, newCond);
						}
						StatementMap targetMap = this.targetMap.getTargetStMap(oldCo, true);
						if(targetMap.getOriginal().equals(oldCo) && targetMap.getNew().equals(newCond) && 
						   (targetMap.getMapType() == MAP_TYPE.UNMODIFIED || targetMap.getMapType() == MAP_TYPE.UNDEFINED)) {
							this.unModifiedConditions.add(oldCo);
						}
					}
					this.conditionsToCheck = condsToCheck;
				}
			}
		} catch(Exception e) {
			e.printStackTrace();
			this.conditionsToCheck = null;
		}
		return this.conditionsToCheck != null;
	}
	
	/***
	 *  Compute for each condition all the statements that could transitively affect each condition.
	 * @return true if successful else false.
	 */
	public boolean computeConditionTransDependency() {
		this.computeConditionsToCheck();
		if(this.oldcondTransDependency == null) {
			this.oldcondTransDependency = computeConditionTransDependency(this.conditionsToCheck.keySet(), this.oldstatementDependency);
			
		}
		if(this.newcondTransDependency == null) {
			this.targetMap.computeInsertedConditions();
			ArrayList<ASTNode> condList = new ArrayList<ASTNode>();
			condList.addAll(this.conditionsToCheck.values());
			condList.addAll(this.targetMap.getInsertedConditions());
			List<ASTNode> tmpCache = new ArrayList<ASTNode>();
			for(ASTNode currNewCond: this.targetMap.getInsertedConditions()) {
				tmpCache.clear();
				getAllBackwardSymbols(currNewCond, this.targetMap.getNewFuncGraphs(), tmpCache, this.newstatementDependency);
			}
			this.newcondTransDependency = computeConditionTransDependency(condList,  this.newstatementDependency);
		}
		
		return this.oldcondTransDependency != null && this.newcondTransDependency != null;
		
	}
	
	public boolean isoldASTNodeAffectingStatement(ASTNode currNode) {
		boolean retVal = false;
		if(currNode != null) {
			this.computeConditionTransDependency();
			for(ASTNode currCondNode: this.conditionsToCheck.keySet()) {
				if(this.oldcondTransDependency.containsKey(currCondNode)) {
					retVal = this.oldcondTransDependency.get(currCondNode).contains(currNode);
					if(retVal) {
						break;
					}
				}
			}
		}
		return retVal;
	}
	
	public boolean isnewASTNodeAffectingStatement(ASTNode currNode) {
		boolean retVal = false;
		if(currNode != null) {
			this.computeConditionTransDependency();
			for(ASTNode currCondNode: this.conditionsToCheck.values()) {
				if(this.newcondTransDependency.containsKey(currCondNode)) {
					retVal = this.newcondTransDependency.get(currCondNode).contains(currNode);
					if(retVal) {
						break;
					}
				}
			}
			for(ASTNode currCondNode: this.targetMap.getInsertedConditions()) {
				if(this.newcondTransDependency.containsKey(currCondNode)) {
					retVal = this.newcondTransDependency.get(currCondNode).contains(currNode);
					if(retVal) {
						break;
					}
				}
			}
		}
		return retVal;
	}
	
	/***
	 * 
	 * @return
	 */
	private boolean computeAffectedVariables() {
		
		try {
			if(this.oldFuncAffectedVars == null || this.newFuncAffectedVars == null) {
				List<StatementMap> allStatementMap = this.targetMap.getStatementMap();
				HashMap<ASTNode, ArrayList<String>> affectedVarsInOld = new HashMap<ASTNode, ArrayList<String>>();
				HashMap<ASTNode, ArrayList<String>> affectedVarsInNew = new HashMap<ASTNode, ArrayList<String>>();
				// for each affected statement, get forward and backward affected symbols.
				ASTNode old = null, newN = null;
				BasicBlock currBB = null;
				List<ASTNode> tmpCache = new ArrayList<ASTNode>();
				for(StatementMap currSt:allStatementMap){
					old = null;
					newN = null;
					if(currSt.getMapType() == MAP_TYPE.UPDATE /*|| currSt.getMapType() == MAP_TYPE.MOVE*/) {
						old = currSt.getOriginal();
						newN = currSt.getNew();
					}
					if(currSt.getMapType() == MAP_TYPE.DELETE) {
						old = currSt.getOriginal();
					}
					if(currSt.getMapType() == MAP_TYPE.INSERT) {
						newN = currSt.getNew();
					}
					if(old != null) {
						tmpCache.clear();
						// Check if the target BB is error, is yes, then ignore the affected statement.
						currBB = this.targetMap.getOldClassicCFG().getBB(old);
						if(!currBB.isConfigErrorHandling()) {
							HashMap<ASTNode, ArrayList<String>> allAffectedSyms = getAllForwardSymbols(old, this.targetMap.getOldFuncGraphs(), tmpCache, this.oldFuncAffectedUseVars, this.oldstatementDependency);
							affectedVarsInOld.putAll(allAffectedSyms);
						}
					}
					if(newN != null) {
						tmpCache.clear();
						// ignore if the target BB is error
						currBB = this.targetMap.getNewClassicCFG().getBB(newN);
						if(!currBB.isConfigErrorHandling()) {
							HashMap<ASTNode, ArrayList<String>> allAffectedSyms = getAllForwardSymbols(newN, this.targetMap.getNewFuncGraphs(), tmpCache, this.newFuncAffectedUseVars, this.newstatementDependency);
							affectedVarsInNew.putAll(allAffectedSyms);
						}
		
					}
				}
				this.oldFuncAffectedVars = affectedVarsInOld;
				this.newFuncAffectedVars = affectedVarsInNew;
			}
		} catch(Exception e) {
			e.printStackTrace();
			this.oldFuncAffectedVars = null;
			this.newFuncAffectedVars = null;
		}
		
		
		return this.oldFuncAffectedVars != null && this.newFuncAffectedVars != null;
	}
	
	/***
	 *  Get all the symbols that are affected (transitively) by the current Node.
	 * 
	 * @param currNode Target node to get the forward symbols of.
	 * @param targetGraphs JoernGraphs of the corresponding function.
	 * @param visited List containing all the visited nodes (to avoid cyclic dependency)
	 * @param affectedUseVars HashMap in which all the used variables should be stored for each ASTNode.
	 * @param statementDependency HashMap of statement dependency of a given statement with other statements.
	 * @return Names of all the affected symbols.
	 */
	private static HashMap<ASTNode, ArrayList<String>> getAllForwardSymbols(ASTNode currNode, 
																			JoernGraphs targetGraphs, 
																			List<ASTNode> visited, 
																			HashMap<ASTNode, HashSet<String>> affectedUseVars,
																			HashMap<ASTNode, HashSet<ASTNode>> statementDependency) {
		HashMap<ASTNode, ArrayList<String>> allForwardSymbols = new HashMap<ASTNode, ArrayList<String>>();
		if(!visited.contains(currNode)) {
			// add node to visited list
			visited.add(currNode);
			DefUseCFG defUseGraph = targetGraphs.getDefUseCfg();
			DDG dataDepenGraph = targetGraphs.getDdg();
			// get all used symbols
			Collection<Object> currDefdSymbols = defUseGraph.getSymbolsDefinedBy(currNode);
			ArrayList<String> allSymbs = new ArrayList<String>();
			// for each of the defined symbol
			for(Object currSymbol: currDefdSymbols) {
				if(currSymbol instanceof String) {
					String currStrSym = (String)currSymbol;
					allSymbs.add(currStrSym);
					// get all the reaching use nodes.
					List<ASTNode> allUseNodes = dataDepenGraph.getReachingUses(currNode, currStrSym);
					// For each of this use nodes.
					// go forward again.
					// for each of the use node, call this function again.
					for(ASTNode currUseNode: allUseNodes) {
						if(!affectedUseVars.containsKey(currUseNode)) {
							affectedUseVars.put(currUseNode, new HashSet<String>());
						}
						affectedUseVars.get(currUseNode).add(currStrSym);
						HashMap<ASTNode, ArrayList<String>> currBkSymbols = getAllForwardSymbols(currUseNode, targetGraphs, visited, affectedUseVars, statementDependency);
						allForwardSymbols.putAll(currBkSymbols);
						
						if(!statementDependency.containsKey(currUseNode)) {
							statementDependency.put(currUseNode, new HashSet<ASTNode>());
						}
						statementDependency.get(currUseNode).add(currNode);
					}
				}
			}
			allForwardSymbols.put(currNode, allSymbs);
		}
		return allForwardSymbols;
	}
	
	private static void getAllBackwardSymbols(ASTNode currNode, JoernGraphs targetGraphs, 
											  List<ASTNode> visited, 
											  HashMap<ASTNode, HashSet<ASTNode>> statementDependency) {
		if(!visited.contains(currNode)) {
			// add node to visited list
			visited.add(currNode);
			DefUseCFG defUseGraph = targetGraphs.getDefUseCfg();
			DDG dataDepenGraph = targetGraphs.getDdg();
			// get all used symbols
			Collection<Object> currDefdSymbols = defUseGraph.getSymbolsUsedBy(currNode);
			ArrayList<String> allSymbs = new ArrayList<String>();
			// for each of the used symbol
			for(Object currSymbol: currDefdSymbols) {
				if(currSymbol instanceof String) {
					String currStrSym = (String)currSymbol;
					allSymbs.add(currStrSym);
					// get all the reaching use nodes.
					List<ASTNode> allDefNodes = dataDepenGraph.getReachingDefs(currNode, currStrSym);
					// For each of this use nodes.
					// go forward again.
					// for each of the use node, call this function again.
					for(ASTNode currUseNode: allDefNodes) {
						if(!statementDependency.containsKey(currNode)) {
							statementDependency.put(currNode, new HashSet<ASTNode>());
						}
						statementDependency.get(currNode).add(currUseNode);
						getAllBackwardSymbols(currUseNode, targetGraphs, visited, statementDependency);				
					}
				}
			}
		}
		
	}
	
	/***
	 *  Get all the transitive dependency for the given node.
	 * @param currNode Node whose transitive dependency need to be fetched.
	 * @param visited Set of visited nodes.
	 * @param condTransDep Map to which the condition dependencies need to be inserted.
	 * @param statDepend Statement level dependency.
	 */
	private static void getAllRecursiveTransDep(ASTNode currNode, HashSet<ASTNode> visited, 
												HashMap<ASTNode, HashSet<ASTNode>> condTransDep, 
												HashMap<ASTNode, HashSet<ASTNode>> statDepend) {
		if(!condTransDep.containsKey(currNode)) {
			if(!visited.contains(currNode)) {
				HashSet<ASTNode> newDepSet = new HashSet<ASTNode>();
				if(statDepend.containsKey(currNode)) {
					visited.add(currNode);
					newDepSet.addAll(statDepend.get(currNode));					
					for(ASTNode currDepNode: statDepend.get(currNode)) {
						if(!visited.contains(currDepNode)) {
							getAllRecursiveTransDep(currDepNode, visited, condTransDep, statDepend);
							newDepSet.addAll(condTransDep.get(currDepNode));
						}
					}					
					visited.remove(currNode);
				}
				condTransDep.put(currNode, newDepSet);
			}
		}
	}
	
	/***
	 *  Compute transitive dependency of the given set of conditions.
	 * @param condtoCheck List of conditions for which the transitive dependencies needs to be computed.
	 * @param statDepend Statement level dependency.
	 * @return Map of conditions to the set of transitive dependency.
	 */
	private static HashMap<ASTNode, HashSet<ASTNode>> computeConditionTransDependency(Collection<ASTNode> condtoCheck, HashMap<ASTNode, HashSet<ASTNode>> statDepend) {
		HashMap<ASTNode, HashSet<ASTNode>> statementTransDependency = new HashMap<ASTNode, HashSet<ASTNode>>();
		HashSet<ASTNode> visitedNodes = new HashSet<ASTNode>();
		for(ASTNode currCond: condtoCheck) {
			getAllRecursiveTransDep(currCond, visitedNodes, statementTransDependency, statDepend);
		}
		return statementTransDependency;
	}
}

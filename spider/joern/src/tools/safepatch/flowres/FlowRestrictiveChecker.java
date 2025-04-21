/**
 * 
 */
package tools.safepatch.flowres;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import cdg.CDG;
import cdg.CDGEdge;

import com.microsoft.z3.Context;

import ddg.DataDependenceGraph.DDG;
import ddg.DefUseCFG.DefUseCFG;
import ast.ASTNode;
import ast.statements.Condition;
import tools.safepatch.cfgclassic.BasicBlock;
import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.fgdiff.StatementMap;
import tools.safepatch.fgdiff.StatementMap.MAP_TYPE;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import tools.safepatch.z3stuff.GlobalZ3ObjManager;
import tools.safepatch.z3stuff.exprhandlers.Z3Utils;

/**
 * @author machiry
 *
 */
public class FlowRestrictiveChecker {
	
	private static final Logger logger = LogManager.getLogger();
	private FunctionMap targetMap = null;
	private HashMap<ASTNode, ArrayList<String>> oldFuncAffectedVars = null;
	private HashMap<ASTNode, ArrayList<String>> newFuncAffectedVars = null;
	private HashMap<ASTNode, ASTNode> conditionsToCheck = null;
	private ArrayList<ASTNode> unModifiedConditions = new ArrayList<ASTNode>(); 
	
	private HashMap<ASTNode, SATISFIABLE_FLAG> condStaisFlags = null;
	private Context ctx = null;
	
	public enum SATISFIABLE_FLAG {
		NEW_IMPL_OLD,
		OLD_IMPL_NEW,
		SAME,
		OTHER
	};
	
	public FlowRestrictiveChecker(FunctionMap currMap) {
		this.targetMap = currMap;
		this.ctx = new Context();
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
	
	public boolean checkMatchedConditionsImplication() {
		this.computeConditionsToCheck();
		
		if(this.condStaisFlags == null) {
			this.condStaisFlags = new HashMap<ASTNode, SATISFIABLE_FLAG>();
			
			GlobalZ3ObjManager.setFunctionMap(this.targetMap);
			// converter for old function
			ASTNodeToZ3Converter oldFuncConverter = new ASTNodeToZ3Converter(this.targetMap, this.targetMap.getOldFuncGraphs(), this.oldFuncAffectedVars, null, true, this.ctx);
			this.targetMap.setOldFnConv(oldFuncConverter);
			// converter for new function
			ASTNodeToZ3Converter newFuncConverter = new ASTNodeToZ3Converter(this.targetMap, this.targetMap.getNewFuncGraphs(), this.newFuncAffectedVars, null, false, this.ctx);
			this.targetMap.setNewFnConv(newFuncConverter);
			this.targetMap.setZ3Ctx(this.ctx);
			
			if(!this.conditionsToCheck.isEmpty()) {
				
				for(ASTNode oldCondNode:this.conditionsToCheck.keySet()) {
					ASTNode newCondNode = this.conditionsToCheck.get(oldCondNode);
					logger.debug("Trying to convert old condition {} to z3 obj", oldCondNode.getEscapedCodeStr());
					Object oldCondZ3Obj = oldFuncConverter.getZ3Expression(oldCondNode, null);
					logger.debug("Trying to convert new condition {} to z3 obj", newCondNode.getEscapedCodeStr());
					Object newCondZ3Obj = newFuncConverter.getZ3Expression(newCondNode, null);
					boolean flag1 = false, flag2 = false;
					if(this.unModifiedConditions.contains(oldCondNode)) {
						flag1 = Z3Utils.checkSatisfiable(this.ctx, oldCondZ3Obj, newCondZ3Obj);
						flag2 = Z3Utils.checkSatisfiable(this.ctx, newCondZ3Obj, oldCondZ3Obj);
					} else {
						flag1  = Z3Utils.checkImplication(this.ctx, oldCondZ3Obj, newCondZ3Obj);
						flag2 = Z3Utils.checkImplication(this.ctx, newCondZ3Obj, oldCondZ3Obj);
					}
					
					if(flag1 && flag2) {
						this.condStaisFlags.put(oldCondNode, SATISFIABLE_FLAG.SAME);
					} else if(flag1) {
						this.condStaisFlags.put(oldCondNode, SATISFIABLE_FLAG.OLD_IMPL_NEW);
					} else if(flag2) {
						this.condStaisFlags.put(oldCondNode, SATISFIABLE_FLAG.NEW_IMPL_OLD);
					} else {
						this.condStaisFlags.put(oldCondNode, SATISFIABLE_FLAG.OTHER);
					}
				}
			}
		}
		return true;
	}
	
	/***
	 * 
	 * @return
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
						newN = currSt.getOriginal();
					}
					if(old != null) {
						tmpCache.clear();
						// Check if the target BB is error, is yes, then ignore the affected statement.
						currBB = this.targetMap.getOldClassicCFG().getBB(old);
						if(!currBB.isConfigErrorHandling() || old instanceof Condition) {
							HashMap<ASTNode, ArrayList<String>> allAffectedSyms = getAllForwardSymbols(old, this.targetMap.getOldFuncGraphs(), tmpCache);
							affectedVarsInOld.putAll(allAffectedSyms);
						}
					}
					if(newN != null) {
						tmpCache.clear();
						// ignore if the target BB is error
						currBB = this.targetMap.getNewClassicCFG().getBB(newN);
						if(!currBB.isConfigErrorHandling() || newN instanceof Condition) {
							HashMap<ASTNode, ArrayList<String>> allAffectedSyms = getAllForwardSymbols(newN, this.targetMap.getNewFuncGraphs(), tmpCache);
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
	 * @return Names of all the affected symbols.
	 */
	private HashMap<ASTNode, ArrayList<String>> getAllForwardSymbols(ASTNode currNode, JoernGraphs targetGraphs, List<ASTNode> visited) {
		HashMap<ASTNode, ArrayList<String>> allForwardSymbols = new HashMap<ASTNode, ArrayList<String>>();
		if(!visited.contains(currNode)) {
			// add node to visited list
			visited.add(currNode);
			DefUseCFG defUseGraph = targetGraphs.getDefUseCfg();
			DDG dataDepenGraph = targetGraphs.getDdg();
			// get all used symbols
			Collection<Object> currDefdSymbols = defUseGraph.getSymbolsDefinedBy(currNode);
			ArrayList<String> allSymbs = new ArrayList<String>();
			// for each of the used symbol
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
						HashMap<ASTNode, ArrayList<String>> currBkSymbols = this.getAllForwardSymbols(currUseNode, targetGraphs, visited);
						allForwardSymbols.putAll(currBkSymbols);
					}
				}
			}
			allForwardSymbols.put(currNode, allSymbs);
		}
		return allForwardSymbols;
	}
}

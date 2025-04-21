/**
 * 
 */
package tools.safepatch.z3stuff;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;

import tools.safepatch.cfgclassic.BasicBlock;
import tools.safepatch.cfgclassic.BasicBlockUtils;
import tools.safepatch.cfgclassic.ClassicCFG;
import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.fgdiff.StatementMap;
import tools.safepatch.flowres.FunctionMap;
import tools.safepatch.z3stuff.exprhandlers.ASTToZ3ExprConvertor;
import tools.safepatch.z3stuff.exprhandlers.Z3Utils;
import ast.ASTNode;
import ast.expressions.CallExpression;
import cdg.CDG;

/**
 * 
 * Main class that handles the conversion of C statements to z3 expressions.
 * @author machiry
 *
 */
public class ASTNodeToZ3Converter {
	private static final Logger logger = LogManager.getLogger();
	// cache that stores all the converted items for the current function.
	private HashMap<ASTNode, HashMap<String, Object>> conversionCache = new HashMap<ASTNode, HashMap<String, Object>>();
	// ASTNodes affected in the current function.
	private HashMap<ASTNode, ArrayList<String>> funcAffectedVars = null;
	// Affected ASTNodes and the corresponding variables used by this statement.
	private HashMap<ASTNode, HashSet<String>> funcAffectedUseVars = null;
	
	private HashSet<ASTNode> transitivelyAffectedVars = null;
	private JoernGraphs functionGraphs = null;
	private boolean isOldFn = true;
	private Context targetCtx = null;
	private FunctionMap targetFnMap = null;
	
	public HashSet<ASTNode> stricterPathVisitedList = new HashSet<ASTNode>();
	
	ASTNodeToZ3Converter oldFuncConverter, newFuncConverter;
	
	public ASTNodeToZ3Converter(FunctionMap targetMap, JoernGraphs currGraphs, HashMap<ASTNode, ArrayList<String>> funcAffectedVars, 
			HashMap<ASTNode, HashSet<String>> funcAffectedUsrVars, 
		boolean isOld, Context ctx) {
		this.functionGraphs = currGraphs;
		this.funcAffectedVars = funcAffectedVars;
		this.funcAffectedUseVars = funcAffectedUsrVars;
		this.isOldFn = isOld;
		this.targetCtx = ctx;
		this.targetFnMap = targetMap;
	}
	
	public ASTNodeToZ3Converter(FunctionMap targetMap, JoernGraphs currGraphs, HashMap<ASTNode, ArrayList<String>> funcAffectedVars, 
								HashMap<ASTNode, HashSet<String>> funcAffectedUsrVars, 
								boolean isOld, Context ctx, HashSet<ASTNode> transAffStmts) {
		this.functionGraphs = currGraphs;
		this.funcAffectedVars = funcAffectedVars;
		this.funcAffectedUseVars = funcAffectedUsrVars;
		this.isOldFn = isOld;
		this.targetCtx = ctx;
		this.transitivelyAffectedVars = transAffStmts;
		this.targetFnMap = targetMap;
	}
	
	public void setTransAffectedVars(HashSet<ASTNode> transAffStmts) {
		this.transitivelyAffectedVars = transAffStmts;
	}
	
	public void setOldFuncConverter(ASTNodeToZ3Converter tmpConv) {
		this.oldFuncConverter = tmpConv;
	}
	
	public void setNewFuncConverter(ASTNodeToZ3Converter tmpConv) {
		this.newFuncConverter = tmpConv;
	}
	
	
	
	/***
	 * 
	 * Convert the given ASTNode to z3 symbol and store into cache.
	 * 
	 * @param currNode Node which needs to be converted to z3 expression.
	 * @param targetSym symbol corresponding to the provided ASTNode.
	 * @return true if successful.
	 */
	public boolean convertToZ3(ASTNode currNode, String targetSym) {
		// check in cache
		if(this.lookupCahce(currNode, targetSym) != null) {
			return true;
		}
		boolean retVal = false;
		try {
			ArrayList<ASTNode> visitedList = new ArrayList<ASTNode>();
			Object targetObj = this.convertASTToZ3Object(currNode, targetSym, visitedList);
			if(targetObj != null) {
				this.putIntoCache(currNode, targetSym, targetObj);
				retVal = true;
			}
		} catch(Exception e) {
			e.printStackTrace();
			retVal = false;
		}
		return retVal;
	}
	
	/***
	 *  Get z3 expression corresponding to the provided ASTNode.
	 * @param currNode ASTNode to be converted to Z3 expression.
	 * @param targetSym Symbol corresponding to the provided ASTNode.
	 * @return z3 object.
	 */
	public Object getZ3Expression(ASTNode currNode, String targetSym) {
		if(this.convertToZ3(currNode, targetSym)) {
			return this.conversionCache.get(currNode).get(targetSym);
		} else {
			logger.error("Unable to get Z3 expression for ASTNode : {} and Symbol: {}", currNode.toString(), targetSym);
		}
		return null;
	}
	
	/***
	 * Recursively convert the provided ASTNode into z3 object.
	 * 
	 * @param currNode target ASTNode to convert.
	 * @param targetSym symbol corresponding to the provided ast node.
	 * @param visitedList List of ASTNodes already visited (to avoid cycles).
	 * @return z3 object.
	 */
	private Object convertASTToZ3Object(ASTNode currNode, String targetSym, ArrayList<ASTNode> visitedList) {
		Object retZ3Obj = this.lookupCahce(currNode, targetSym);
		if(retZ3Obj == null) {
			if(!visitedList.contains(currNode)) {
				visitedList.add(currNode);
				boolean generateNewVar = false;
				// is this symbol from old function?
				boolean fromOld = this.isOldFn;
				// Do we have to generate new Z3 symbol for this?
				if(this.funcAffectedVars.containsKey(currNode) || this.transitivelyAffectedVars.contains(currNode)) {
				// Yes, because this ASTNode is somehow affected by the changes.
					generateNewVar = true;
					/*if(!this.funcAffectedVars.get(currNode).contains(targetSym)) {
						logger.error("Node is affected {} but the required symbol {} is not modified.",currNode.getEscapedCodeStr(), targetSym);
					}*/
				}
				// First check global cache
				retZ3Obj = GlobalZ3ObjManager.getZ3ObjSymbol(currNode, targetSym, fromOld);
				if(retZ3Obj != null) {
					visitedList.remove(currNode);
					return retZ3Obj;
				}
				ASTToZ3ExprConvertor targetHandler = new ASTToZ3ExprConvertor(this.functionGraphs, currNode, this.targetCtx, this);
				// Compute the required values
				Map<ASTNode, ArrayList<String>> allVals = targetHandler.getRequiredValues();
				if(allVals != null) {
					HashMap<String, Object> targetVals = new HashMap<String, Object>();
					HashMap<String, HashSet<ASTNode>> duplicateSymbols = new HashMap<String, HashSet<ASTNode>>();
					for(ASTNode childAST: allVals.keySet()) {
						for(String childSymName: allVals.get(childAST)) {
							Object targetChObj = this.convertASTToZ3Object(childAST, childSymName, visitedList);
							if(!duplicateSymbols.containsKey(childSymName)) {
								duplicateSymbols.put(childSymName, new HashSet<ASTNode>());
							}
							duplicateSymbols.get(childSymName).add(childAST);
							targetVals.put(childSymName, targetChObj);
						}
					}
					// for all multiple definitions, create a new variable.
					for(String currDuplSymbol: duplicateSymbols.keySet()) {
						if(duplicateSymbols.get(currDuplSymbol).size() > 1) {
							targetVals.remove(currDuplSymbol);
							Object targetChObj = this.generateMultiDefStringSymbol(currNode, duplicateSymbols.get(currDuplSymbol), currDuplSymbol);
							targetVals.put(currDuplSymbol, targetChObj);
						}
						
					}
					// provide the requested values
					targetHandler.putRequiredValues(currNode, targetVals);
				}
				
				// save the intermediate results into global cache
				Map<String, Object> allInteralue = targetHandler.getAllIntermediateValues();
				if(allInteralue != null) {
					for(String currSymName:allInteralue.keySet()) {
						GlobalZ3ObjManager.putZ3ObjSymbol(currNode, currSymName, allInteralue.get(currSymName), fromOld, generateNewVar);
					}
				}
				// process the main guy
				retZ3Obj = targetHandler.processASTNode(targetSym);
				if(retZ3Obj != null) {
					GlobalZ3ObjManager.putZ3ObjSymbol(currNode, targetSym, retZ3Obj, fromOld, generateNewVar);
				}
				visitedList.remove(currNode);
			} else {
				logger.warn("Recursion detected, returning a top guy for {}", currNode.getEscapedCodeStr());
				retZ3Obj = Z3Utils.getTop(this.targetCtx);
			}
		}
		return retZ3Obj;
	}
	
	/***
	 *  Generate z3 object for a call expression.
	 *  
	 * @param targetExpr CallExpression to convert to z3.
	 * @param symName symbol name corresponding to the call instruction.
	 * @return z3 object.
	 */
	private Object generateCallExpressionStringSymbol(CallExpression targetExpr, String symName) {
		
		Object retVal = this.lookupCahce(targetExpr, symName);
		if(retVal == null) {
			
			// check global cache
			retVal = GlobalZ3ObjManager.getZ3ObjSymbol(targetExpr, symName, this.isOldFn);
			if(retVal == null) {
				retVal = Z3Utils.getTop(this.targetCtx);
				
				boolean unChanged = true;
				
				ArrayList<String> affectedSyms = new ArrayList<String>();
				if(this.funcAffectedVars.containsKey(targetExpr)) {
					affectedSyms = this.funcAffectedVars.get(targetExpr);
				}
				
				
				for(int i=1; i < targetExpr.getChildCount(); i++) {
					ASTNode currChild = targetExpr.getChild(i);
					Collection<Object> allSymsUSed = this.functionGraphs.getDefUseCfg().getIdentifierSymbolsUsedBy(currChild);
					for(Object currSym: allSymsUSed) {
						if(currSym instanceof String) {
							if(affectedSyms.contains((String)currSym)) {
								unChanged = false;
								break;
							}
						}
					}
					if(!unChanged) {
						break;
					}
				}
				
				GlobalZ3ObjManager.putZ3ObjSymbol(targetExpr, symName, retVal, this.isOldFn, unChanged);
			}
		}
		
		return retVal;
	}
	
	/***
	 *  Generate a new unconstrained z3 object for the given symbol at the corresponding statement node.
	 *  
	 * @param statementNode Statement node where the z3 object should be created.
	 * @param symName symbol name corresponding to the z3 object.
	 * @return target z3 object.
	 */
	public Object generateStringSymbol(ASTNode statementNode, String symName) {
		
		if(statementNode instanceof CallExpression) {
			return this.generateCallExpressionStringSymbol((CallExpression)statementNode, symName);
		}
		Object retVal = this.lookupCahce(statementNode, symName);
		if(retVal == null) {
			// check global cache
			retVal = GlobalZ3ObjManager.getZ3ObjSymbol(statementNode, symName, this.isOldFn);
			if(retVal == null) {
				// Okay, new value needs to be created.
				retVal = Z3Utils.getTop(this.targetCtx);
				boolean isAffectedVar = this.funcAffectedUseVars.containsKey(statementNode) && 
										this.funcAffectedUseVars.get(statementNode).contains(symName);
				GlobalZ3ObjManager.putZ3ObjSymbol(statementNode, symName, retVal, this.isOldFn, isAffectedVar);
			}
		}
		return retVal;
	}
	
	private boolean checkStricterPath(List<ASTNode> srcNodes, ASTNode srcTarNode, ClassicCFG srcClassicCFG, ASTNodeToZ3Converter srcConverter, 
									  List<ASTNode> dstNodes, ASTNode dstTarNode, ClassicCFG dstClassicCFG, ASTNodeToZ3Converter dstConverter) {
		boolean retVal = true;
		if(dstNodes.size() >= srcNodes.size()) {
			// optimization: only if one of the condition is affected.
			boolean toCheckImpl = false;			
			List<ASTNode> toCheck;
			if(this.isOldFn) {
				toCheck = srcNodes;
			} else {
				toCheck = dstNodes;
			}
			for(ASTNode currA: toCheck) {
				if(this.funcAffectedUseVars.containsKey(currA)) {
					toCheckImpl = true;
					break;
				}
			}
			
			if(toCheckImpl) {			
				BasicBlock srcTarBB = srcClassicCFG.getBB(srcTarNode);
				BasicBlock dstTarBB = dstClassicCFG.getBB(dstTarNode);
				Object srcZ3Cond = null;
				Object dstZ3Cond = null;
				Object tmpVal;
				for(ASTNode currN: srcNodes) {
					if(!srcConverter.stricterPathVisitedList.contains(currN)) {
						srcConverter.stricterPathVisitedList.add(currN);
						tmpVal = srcConverter.getZ3Expression(currN, null);
						if(tmpVal != null) {
							BasicBlock srcTmpBB = srcClassicCFG.getBB(currN);
							String tarSt = BasicBlockUtils.getFirstEdgeLabel(srcTmpBB, srcTarBB);
							if(tarSt != null && tarSt.equals("false")) {
								tmpVal = this.targetCtx.mkNot((BoolExpr)tmpVal);
							}
							if(srcZ3Cond == null) {
								srcZ3Cond = tmpVal;
							} else {
								srcZ3Cond = this.targetCtx.mkAnd((BoolExpr)srcZ3Cond, (BoolExpr)tmpVal);
							}
						}
						srcConverter.stricterPathVisitedList.remove(currN);
					}
				}
				
				for(ASTNode currN: dstNodes) {
					if(!dstConverter.stricterPathVisitedList.contains(currN)) {
						dstConverter.stricterPathVisitedList.add(currN);
						tmpVal = dstConverter.getZ3Expression(currN, null);
						if(tmpVal != null) {
							BasicBlock srcTmpBB = dstClassicCFG.getBB(currN);
							String tarSt = BasicBlockUtils.getFirstEdgeLabel(srcTmpBB, dstTarBB);
							if(tarSt != null && tarSt.equals("false")) {
								tmpVal = this.targetCtx.mkNot((BoolExpr)tmpVal);
							}
							if(dstZ3Cond == null) {
								dstZ3Cond = tmpVal;
							} else {
								dstZ3Cond = this.targetCtx.mkAnd((BoolExpr)dstZ3Cond, (BoolExpr)tmpVal);
							}
						}
						dstConverter.stricterPathVisitedList.remove(currN);
					}
				}
				
				if(srcZ3Cond != null && dstZ3Cond != null) {
					retVal = Z3Utils.checkImplication(this.targetCtx, srcZ3Cond, dstZ3Cond);
				}
			}
			
			
		}
		return retVal;
	}
	
	private boolean checkPathEquivalence(HashSet<ASTNode> currNodes) {
		boolean retVal = true;
		
		List<ASTNode> oldCDNodes, newCDNodes;
		CDG oldCDG, newCDG;
		ClassicCFG oldCFG, newCFG;
		oldCDG = this.targetFnMap.getOldFuncGraphs().getCdg();
		newCDG = this.targetFnMap.getNewFuncGraphs().getCdg();
		oldCFG = this.targetFnMap.getOldClassicCFG();
		newCFG = this.targetFnMap.getNewClassicCFG();
		
		for(ASTNode currTmpNode: currNodes) {
			StatementMap tarStMap = this.targetFnMap.getTargetStMap(currTmpNode, this.isOldFn);
			ASTNode mappedNode;
			if(this.isOldFn) {
				mappedNode = tarStMap.getNew();
			} else {
				mappedNode = tarStMap.getOriginal();
			}
			if(mappedNode == null) {
				retVal = false;
				break;
			}
			ASTNode oldChkNode, newChkNode;
			if(this.isOldFn) {
				oldCDNodes = oldCDG.getPathConstraintList(currTmpNode);
				newCDNodes = newCDG.getPathConstraintList(mappedNode);
				oldChkNode = currTmpNode;
				newChkNode = mappedNode;
			} else {
				newCDNodes = newCDG.getPathConstraintList(currTmpNode);
				oldCDNodes = oldCDG.getPathConstraintList(mappedNode);
				oldChkNode = mappedNode;
				newChkNode = currTmpNode;
			}
			
			if(!this.checkStricterPath(oldCDNodes, oldChkNode, oldCFG, this.oldFuncConverter, 
									  newCDNodes, newChkNode, newCFG, this.newFuncConverter)) {
				retVal = false;
				break;
			}
			
			
		}
		
		return retVal;
	}
	
	
	/***
	 *  Generate symbol corresponding to multiple ASTNodes. 
	 *  This is needed for symbols which has multiple definitions that can
	 *  reach a statement.
	 *  
	 * @param statementNode target statement node.
	 * @param targetDefNodes Set of ASTNodes that define the provided symbol.
	 * @param symName target symbol name.
	 * @return z3 object.
	 */
	public Object generateMultiDefStringSymbol(ASTNode statementNode, 
											   HashSet<ASTNode> targetDefNodes, 
											   String symName) {
		
		Object retVal = this.lookupCahce(statementNode, symName);
		if(retVal == null) {
			// check global cache
			retVal = GlobalZ3ObjManager.getMultidefZ3ObjSymbol(statementNode, targetDefNodes, symName, this.isOldFn);
			if(retVal == null) {
				// Okay, new value needs to be created.
				retVal = Z3Utils.getTop(this.targetCtx);
				boolean isAffectedVar = this.funcAffectedUseVars.containsKey(statementNode) && 
										this.funcAffectedUseVars.get(statementNode).contains(symName);
				if(!isAffectedVar) {
					isAffectedVar = !this.checkPathEquivalence(targetDefNodes);
				}
				GlobalZ3ObjManager.putMultidefZ3ObjSymbol(statementNode, targetDefNodes, symName, retVal, this.isOldFn, isAffectedVar);
			}
		}
		return retVal;
	}
	
	/***
	 *  Generate a z3 object for the provided identifier.
	 * @param statementNode ASTNode corresponding to the required identifier.
	 * @param symName symbol name of the identifier.
	 * @return z3 object corresponding to the identifier.
	 */
	public Object generateIdentifierSymbol(ASTNode statementNode, String symName) {
		// for all single use identifiers..we use the same symbolic value.
		// in the old and the new function.
		Object retVal = this.lookupCahce(statementNode, symName);
		if(retVal == null) {
			retVal = GlobalZ3ObjManager.getIdentifierSymbol(this.targetCtx, symName);
			if(retVal == null) {
				retVal = Z3Utils.getTop(this.targetCtx);
				logger.debug("Creating new identifer symbol for {}", symName);
				GlobalZ3ObjManager.putIdentifierSymbol(symName, retVal);
			}
		}
		return retVal;
	}
	
	/***
	 * Look up conversion cache.
	 * 
	 * @param currNode target ASTNode
	 * @param targetSym symbol name
	 * @return z3 object if it is present in cache else null.
	 */
	private Object lookupCahce(ASTNode currNode, String targetSym) {
		if(this.conversionCache.containsKey(currNode)) {
			HashMap<String, Object> targetVarMap = this.conversionCache.get(currNode);
			if(targetVarMap.containsKey(targetSym)) {
				return targetVarMap.get(targetSym);
			}
		}
		return null;
	}
	
	/***
	 *  Store the created z3 object into conversion cache.
	 *  
	 * @param currNode current Node to convert.
	 * @param targetSym symbol to check.
	 * @param targetObj to store z3 object.
	 */
	private void putIntoCache(ASTNode currNode, String targetSym, Object targetObj) {
		if(targetObj != null) {
			HashMap<String, Object> targetVarMap = new HashMap<String, Object>();
			if(this.conversionCache.containsKey(currNode)) {
				targetVarMap = this.conversionCache.get(currNode);
			} else {
				this.conversionCache.put(currNode, targetVarMap);
			}
			targetVarMap.put(targetSym, targetObj);
		}
	}
}

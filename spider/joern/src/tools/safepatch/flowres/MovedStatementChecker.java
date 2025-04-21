/**
 * 
 */
package tools.safepatch.flowres;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.sun.org.apache.xpath.internal.operations.Bool;

import ast.ASTNode;
import ast.statements.Condition;
import ast.statements.ForInit;
import ast.statements.Statement;
import tools.safepatch.cfgclassic.BasicBlock;
import tools.safepatch.cfgclassic.BasicBlockUtils;
import tools.safepatch.cfgclassic.ClassicCFG;
import tools.safepatch.fgdiff.StatementMap;
import tools.safepatch.fgdiff.StatementMap.MAP_TYPE;
import tools.safepatch.heuristics.CodeHeuristics;
import tools.safepatch.z3stuff.ASTNodeToZ3Converter;
import tools.safepatch.z3stuff.exprhandlers.Z3Utils;

/**
 * @author machiry
 *
 */
public class MovedStatementChecker {
	private FunctionMap targetMap = null;
	private static final Logger logger = LogManager.getLogger();
	
	public MovedStatementChecker(FunctionMap currMap) {
		this.targetMap = currMap;
	}
	
	private ArrayList<ASTNode> getCDASTNodeList(BasicBlock currBB, ArrayList<BasicBlock> visitedBB) {
		ArrayList<ASTNode> toRet = new ArrayList<ASTNode>();
		if(!visitedBB.contains(currBB)) {
			visitedBB.add(currBB);
			ArrayList<ASTNode> childList = null;
			if(currBB.cdBBs.size() > 0) {
				BasicBlock currChildCD = currBB.cdBBs.get(0);
				toRet.add(currChildCD.getLastNode());
				childList = getCDASTNodeList(currChildCD, visitedBB);
				for(ASTNode childAST:childList) {
					toRet.add(childAST);
				}				
			}
			visitedBB.remove(currBB);
		}
		return toRet;
	}
	
	public boolean areAnyStatementsMoved(PrintWriter jsonWriter) { 
		boolean retVal = false;
		List<StatementMap> allStMap = this.targetMap.getStatementMap();
		logger.debug("Starting moved statement checker\n");
		List<StatementMap> targetStatements = new ArrayList<StatementMap>();
		for(StatementMap currStMap:allStMap) {
			if(currStMap.getMapType() == MAP_TYPE.MOVE) {
				targetStatements.add(currStMap);
			}
		}
		
		jsonWriter.write("\"numMoved\":" + Integer.toString(targetStatements.size()) + ",");
		if(targetStatements.size() > 0) {
			retVal = true;
			logger.debug("Identified {} number of moved statemnents", targetStatements.size());
			HashMap<BasicBlock, BasicBlock> verifiedBBs = new HashMap<BasicBlock, BasicBlock>();
			// analyze the move statements
			for(StatementMap currMap:targetStatements) {
				ASTNode oldSt = currMap.getOriginal();
				ASTNode newSt = currMap.getNew();
				BasicBlock oldBB = this.targetMap.getOldClassicCFG().getBB(oldSt);
				BasicBlock newBB = this.targetMap.getNewClassicCFG().getBB(newSt);
				
				if(oldBB.isConfigErrorHandling() || newBB.isConfigErrorHandling()) {
					logger.debug("One of the statements {} and {} has moved in to error handling basicblock, Ignoring", oldSt.getEscapedCodeStr(), newSt.getEscapedCodeStr());
					continue;
				}
				return true;
			}
		}
		
		return false;
		
	}
	
	private static Object getCDListToZ3Cond(List<ASTNode> targetASTList, BasicBlock dstBB, ClassicCFG targetClassicCFG, ASTNodeToZ3Converter targetConv, Context ctx) {
		Object retVal = null;
		if(targetASTList.size() > 0) {
			for(int i=targetASTList.size() - 1; i >= 0; i--) {
				ASTNode currNode = targetASTList.get(i);
				BasicBlock currBB = targetClassicCFG.getBB(currNode);
				boolean addNot = false;
				String targetEdgeLabel = BasicBlockUtils.getFirstEdgeLabel(currBB, dstBB);
				if(targetEdgeLabel != null) {
					targetEdgeLabel = targetEdgeLabel.trim().toLowerCase();
					addNot = targetEdgeLabel.equals("false");
				}
				Object currCond = targetConv.getZ3Expression(currNode, null);
				
				if(addNot) {
					currCond = ctx.mkNot((BoolExpr)currCond);
				}
				
				if(retVal == null) {
					retVal = currCond;
				} else {
					retVal = ctx.mkAnd((BoolExpr)retVal, (BoolExpr)currCond);
				}
			}
		} else {
			retVal = ctx.mkBool(true);
		}
		return retVal;
		
	}
	
	private static boolean checkMoreRestrictiveCondtions(List<ASTNode> oldCondList, 
														 List<ASTNode> newCondList) {
		boolean retVal = false;
		if(oldCondList.size() <= newCondList.size()) {
			retVal = true;
			ArrayList<String> oldSL = new ArrayList<String>();
			ArrayList<String> newSL = new ArrayList<String>();
			
			for(ASTNode n:oldCondList) {
				if(n != null) {
					oldSL.add(n.getEscapedCodeStr());
				}
			}
			
			for(ASTNode n:newCondList) {
				if(n != null) {
					newSL.add(n.getEscapedCodeStr());
				}
			}
			
			for(String currS: oldSL) {
				int newSLI = newSL.indexOf(currS);
				if(newSLI >= 0) {
					newSL.remove(newSLI);
				} else {
					retVal = false;
					break;
				}
			}
		}		
		return retVal;
	}
	
	 
	
	public boolean areMovedStatementsSafe(PrintWriter jsonWriter) {
		boolean retVal = false;
		List<StatementMap> allStMap = this.targetMap.getStatementMap();
		logger.debug("Starting moved statement checker\n");
		List<StatementMap> targetStatements = new ArrayList<StatementMap>();
		for(StatementMap currStMap:allStMap) {
			if(currStMap.getMapType() == MAP_TYPE.MOVE) {
				targetStatements.add(currStMap);
			}
		}
		
		jsonWriter.write("\"numMoved\":" + Integer.toString(targetStatements.size()) + ",");
		if(targetStatements.size() > 0) {
			retVal = true;
			logger.debug("Identified {} number of moved statemnents", targetStatements.size());
			HashMap<BasicBlock, BasicBlock> verifiedBBs = new HashMap<BasicBlock, BasicBlock>();
			// analyze the move statements
			for(StatementMap currMap:targetStatements) {
				ASTNode oldSt = currMap.getOriginal();
				ASTNode newSt = currMap.getNew();
				BasicBlock oldBB = this.targetMap.getOldClassicCFG().getBB(oldSt);
				BasicBlock newBB = this.targetMap.getNewClassicCFG().getBB(newSt);
				
				if(oldBB.isConfigErrorHandling() || newBB.isConfigErrorHandling()) {
					logger.debug("One of the statements {} and {} has moved in to error handling basicblock, Ignoring", oldSt.getEscapedCodeStr(), newSt.getEscapedCodeStr());
					continue;
				}
				
				if(oldSt instanceof Condition && newSt instanceof Condition) {
					logger.debug("Ignoring moved conditions {}", oldSt.getEscapedCodeStr());
					continue;
				}
				
				if(verifiedBBs.containsKey(oldBB) && verifiedBBs.get(oldBB) == newBB) {
					continue;
				}
				
				logger.debug("Getting CD ASTNode list for old condition {}", oldSt.getEscapedCodeStr());
				ArrayList<ASTNode> oldCDList = this.getCDASTNodeList(oldBB, new ArrayList<BasicBlock>());
				
				logger.debug("Getting CD ASTNode list for new condition {}", newSt.getEscapedCodeStr());
				ArrayList<ASTNode> newCDList = this.getCDASTNodeList(newBB, new ArrayList<BasicBlock>());
				
				
				/*logger.debug("Converting the old CD list of Z3 conditon list");
				Object oldCondList = MovedStatementChecker.getCDListToZ3Cond(oldCDList, oldBB, this.targetMap.getOldClassicCFG(), 
																			 this.targetMap.getOldFnConv(), this.targetMap.getZ3Ctx());
				
				logger.debug("Converting the new CD list of Z3 conditon list");
				Object newCondList = MovedStatementChecker.getCDListToZ3Cond(newCDList, newBB, this.targetMap.getNewClassicCFG(), 
																			 this.targetMap.getNewFnConv(), this.targetMap.getZ3Ctx());
				
				// Here new should imply old.
				logger.debug("Checking implication..");
				boolean hasImplication = Z3Utils.checkImplication(this.targetMap.getZ3Ctx(), newCondList, oldCondList);
				if(hasImplication) {
					logger.debug("Conditions ({}) and ({}) imply each other", newCondList.toString(), oldCondList.toString());
					verifiedBBs.put(newBB, oldBB);
				} else {
					logger.debug("Conditions ({}) and ({}) DO NOT imply each other", newCondList.toString(), oldCondList.toString());
					retVal = false;
				}*/
				boolean hasImplication = MovedStatementChecker.checkMoreRestrictiveCondtions(oldCDList, newCDList);
				// either the statement has been moved to more restrictive or this is a safe statement
				CodeHeuristics currHeu = new CodeHeuristics();
				if(hasImplication || (newSt instanceof ForInit)) {
					logger.debug("Statment {} moved to more restrictive path", oldSt.getEscapedCodeStr());
					verifiedBBs.put(newBB, oldBB);
				} else if((newSt instanceof Statement) && currHeu.isInitNew((Statement)newSt)) {
					logger.debug("Statment {} moved to more restrictive path", oldSt.getEscapedCodeStr());
					verifiedBBs.put(newBB, oldBB);
				} else {
					logger.debug("Statement {} moved to less restrictive location", oldSt.getEscapedCodeStr());
					retVal = false;
				}
				
				
			}
		} else {
			logger.debug("Found no moved statements\n");
			retVal = true;
		}
		jsonWriter.write("\"moveResult\":" + Boolean.toString(retVal) + ",");
		
		return retVal;
	}
	
	public boolean areMovedStatementsSafe(PrintWriter jsonWriter, Collection<StatementMap> targetStatements) {
		boolean retVal = false;
		logger.debug("Starting moved statement checker\n");
		
		jsonWriter.write("\"numMoved\":" + Integer.toString(targetStatements.size()) + ",");
		if(targetStatements.size() > 0) {
			retVal = true;
			logger.debug("Identified {} number of moved statemnents", targetStatements.size());
			HashMap<BasicBlock, BasicBlock> verifiedBBs = new HashMap<BasicBlock, BasicBlock>();
			// analyze the move statements
			for(StatementMap currMap:targetStatements) {
				ASTNode oldSt = currMap.getOriginal();
				ASTNode newSt = currMap.getNew();
				BasicBlock oldBB = this.targetMap.getOldClassicCFG().getBB(oldSt);
				BasicBlock newBB = this.targetMap.getNewClassicCFG().getBB(newSt);
				
				if(oldBB.isConfigErrorHandling() || newBB.isConfigErrorHandling()) {
					logger.debug("One of the statements {} and {} has moved in to error handling basicblock, Ignoring", oldSt.getEscapedCodeStr(), newSt.getEscapedCodeStr());
					continue;
				}
				
				if(oldSt instanceof Condition && newSt instanceof Condition) {
					logger.debug("Ignoring moved conditions {}", oldSt.getEscapedCodeStr());
					continue;
				}
				
				if(verifiedBBs.containsKey(oldBB) && verifiedBBs.get(oldBB) == newBB) {
					continue;
				}
				
				logger.debug("Getting CD ASTNode list for old condition {}", oldSt.getEscapedCodeStr());
				ArrayList<ASTNode> oldCDList = this.getCDASTNodeList(oldBB, new ArrayList<BasicBlock>());
				
				logger.debug("Getting CD ASTNode list for new condition {}", newSt.getEscapedCodeStr());
				ArrayList<ASTNode> newCDList = this.getCDASTNodeList(newBB, new ArrayList<BasicBlock>());
				
				
				/*logger.debug("Converting the old CD list of Z3 conditon list");
				Object oldCondList = MovedStatementChecker.getCDListToZ3Cond(oldCDList, oldBB, this.targetMap.getOldClassicCFG(), 
																			 this.targetMap.getOldFnConv(), this.targetMap.getZ3Ctx());
				
				logger.debug("Converting the new CD list of Z3 conditon list");
				Object newCondList = MovedStatementChecker.getCDListToZ3Cond(newCDList, newBB, this.targetMap.getNewClassicCFG(), 
																			 this.targetMap.getNewFnConv(), this.targetMap.getZ3Ctx());
				
				// Here new should imply old.
				logger.debug("Checking implication..");
				boolean hasImplication = Z3Utils.checkImplication(this.targetMap.getZ3Ctx(), newCondList, oldCondList);
				if(hasImplication) {
					logger.debug("Conditions ({}) and ({}) imply each other", newCondList.toString(), oldCondList.toString());
					verifiedBBs.put(newBB, oldBB);
				} else {
					logger.debug("Conditions ({}) and ({}) DO NOT imply each other", newCondList.toString(), oldCondList.toString());
					retVal = false;
				}*/
				boolean hasImplication = MovedStatementChecker.checkMoreRestrictiveCondtions(oldCDList, newCDList);
				// either the statement has been moved to more restrictive or this is a safe statement
				CodeHeuristics currHeu = new CodeHeuristics();
				if(hasImplication || (newSt instanceof ForInit)) {
					logger.debug("Statment {} moved to more restrictive path", oldSt.getEscapedCodeStr());
					verifiedBBs.put(newBB, oldBB);
				} else if((newSt instanceof Statement) && currHeu.isInitNew((Statement)newSt)) {
					logger.debug("Statment {} moved to more restrictive path", oldSt.getEscapedCodeStr());
					verifiedBBs.put(newBB, oldBB);
				} else {
					logger.debug("Statement {} moved to less restrictive location", oldSt.getEscapedCodeStr());
					retVal = false;
				}
				
				
			}
		} else {
			logger.debug("Found no moved statements\n");
			retVal = true;
		}
		jsonWriter.write("\"moveResult\":" + Boolean.toString(retVal) + ",");
		
		return retVal;
	}
	
}

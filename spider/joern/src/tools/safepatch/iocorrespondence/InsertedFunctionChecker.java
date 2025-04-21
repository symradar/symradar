/**
 * 
 */
package tools.safepatch.iocorrespondence;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ast.ASTNode;
import ast.expressions.CallExpression;
import ast.statements.Statement;
import cfg.nodes.CFGNode;
import tools.safepatch.cfgclassic.BasicBlock;
import tools.safepatch.cfgclassic.ClassicCFG;
import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.fgdiff.StatementMap;
import tools.safepatch.flowres.FunctionMap;
import tools.safepatch.heuristics.CodeHeuristics;

/**
 * This class checks the effect of our heuristics on inserted function calls.
 * 
 * 
 * @author machiry
 *
 */
public class InsertedFunctionChecker {
	private static final Logger logger = LogManager.getLogger("InsertedFuncChecker");
	private FunctionMap funcMap;
	
	public InsertedFunctionChecker(FunctionMap fmap) {
		org.apache.logging.log4j.core.config.Configurator.setLevel("InsertedFuncChecker", Level.OFF);
		this.funcMap = fmap;
	}
	
	private boolean isFunctionLock(CallExpression call) {
		String callee = call.getChild(0).getEscapedCodeStr().toLowerCase().trim();
		
		return callee.endsWith("_" + CodeHeuristics.LOCK) ||  callee.startsWith(CodeHeuristics.LOCK + "_") || 
				callee.contains("_" + CodeHeuristics.LOCK + "_");
	}
	
	private boolean isFunctionUnLock(CallExpression call) {
		String callee = call.getChild(0).getEscapedCodeStr().toLowerCase().trim();
		
		return callee.endsWith("_" + CodeHeuristics.UNLOCK) ||  callee.startsWith(CodeHeuristics.UNLOCK + "_") || 
				callee.contains("_" + CodeHeuristics.UNLOCK + "_");
	}
	
	private Set<BasicBlock> getPreDominatorBBs(CFGNode srcNode, JoernGraphs jg, ClassicCFG ccfg) { 
		HashSet<BasicBlock> toRet = new HashSet<BasicBlock>();
		for(CFGNode cn: jg.getDomTree().getAllDominators(srcNode, null)) {
			toRet.add(ccfg.getBB(cn));
		}
		return toRet;
	}
	
	private Set<BasicBlock> getPostDominatorBBs(CFGNode srcNode, JoernGraphs jg, ClassicCFG ccfg) { 
		HashSet<BasicBlock> toRet = new HashSet<BasicBlock>();
		for(CFGNode cn: jg.getPostDomTree().getAllDominators(srcNode, null)) {
			toRet.add(ccfg.getBB(cn));
		}
		return toRet;
	}
	
	private void getMappedASTNodes(Set<BasicBlock> newFuncBBs, ArrayList<ASTNode> oldNodes, ArrayList<ASTNode> newNodes) {
		for(BasicBlock currB: newFuncBBs) {
			for(ASTNode newAs: currB.getAllASTNodes()) {
				newNodes.add(newAs);
				StatementMap sm = this.funcMap.getTargetStMap(newAs, false);
				if(sm != null && sm.getOriginal() != null) {
					oldNodes.add(sm.getOriginal());
				}
			}
		}
	}
	private boolean checkUnlockPresent(Set<BasicBlock> newFuncBBs) {
		ArrayList<ASTNode> oldNodes = new ArrayList<ASTNode>();
		ArrayList<ASTNode> newNodes = new ArrayList<ASTNode>();
		getMappedASTNodes(newFuncBBs, oldNodes, newNodes);
		boolean unlockInOld = false;
		boolean unlockInNew = false;
		for(ASTNode o: oldNodes) {
			if(o instanceof Statement) {
				Statement s = (Statement)o;
				if(s.getChild(0) instanceof CallExpression) {
					CallExpression cexpr = (CallExpression)s.getChild(0);
					if(isFunctionUnLock(cexpr)) {
						unlockInOld = true;
						break;
					}
				}
			}
		}
		if(unlockInOld) {
			for(ASTNode o: newNodes) {
				if(o instanceof Statement) {
					Statement s = (Statement)o;
					if(s.getChild(0) instanceof CallExpression) {
						CallExpression cexpr = (CallExpression)s.getChild(0);
						if(isFunctionUnLock(cexpr)) {
							unlockInNew = true;
							break;
						}
					}
				}
			}
		}
		return unlockInOld && unlockInNew;
	}
	
	private boolean checkLockPresent(Set<BasicBlock> newFuncBBs) {
		ArrayList<ASTNode> oldNodes = new ArrayList<ASTNode>();
		ArrayList<ASTNode> newNodes = new ArrayList<ASTNode>();
		getMappedASTNodes(newFuncBBs, oldNodes, newNodes);
		boolean lockInOld = false;
		boolean lockInNew = false;
		for(ASTNode o: oldNodes) {
			if(o instanceof Statement) {
				Statement s = (Statement)o;
				if(s.getChild(0) instanceof CallExpression) {
					CallExpression cexpr = (CallExpression)s.getChild(0);
					if(isFunctionLock(cexpr)) {
						lockInOld = true;
						break;
					}
				}
			}
		}
		
		if(lockInOld) {
			for(ASTNode o: newNodes) {
				if(o instanceof Statement) {
					Statement s = (Statement)o;
					if(s.getChild(0) instanceof CallExpression) {
						CallExpression cexpr = (CallExpression)s.getChild(0);
						if(isFunctionLock(cexpr)) {
							lockInNew = true;
							break;
						}
					}
				}
			}
		}
		
		return lockInOld && lockInNew;
	}
	
	/*
	 * If the inserted call is to mutex_lock and mutex_unlock => make sure that unlock 
	 * is in the post-dominator BB of that of mutex_lock BB.
	 * 
	 * If the inserted call is to mutex_lock, make sure that there is 
	 * mutex_unlock in a post-dominator BB in both old and new function.
	 * 
	 * If the inserted call is to mutex_unlock, make sure the there 
	 * is a call to mutex_lock in a pre-dominator BB.
	 */
	
	public boolean checkInsertedFunctionSanity(List<Statement> allInsertedStmts) {
		boolean retVal = true;
		
		List<CallExpression> insertedCallExprs = new ArrayList<CallExpression>();
		for(Statement stmt: allInsertedStmts) {
			if(stmt.getChild(0) instanceof CallExpression) {
				insertedCallExprs.add((CallExpression)stmt.getChild(0));
			}
		}
		
		// if there are no inserted call expressions.
		// everything is fine.
		if(!insertedCallExprs.isEmpty()) {
			boolean hasLock = false, hasUnLock = false;
			CallExpression lockCall = null;
			CallExpression unlockCall = null;
			
			for(CallExpression ce: insertedCallExprs) {
				if(isFunctionLock(ce)) {
					hasLock = true;
					lockCall = ce;
				}
				if(isFunctionUnLock(ce)) {
					hasUnLock = true;
					unlockCall = ce;
				}
			}
			
			// if we have inserted both lock and unlock..we are fine.
			if(!(hasLock && hasUnLock)) {
				if(hasLock) {
					// get all the post-dominator BBs 
					// in the newBBs and oldBBs
					// this is in new function
					Statement callStmt = (Statement)lockCall.getParent();
					BasicBlock tarBB = this.funcMap.getNewClassicCFG().getBB(callStmt);
					Set<BasicBlock> newFuncBBs = getPostDominatorBBs(tarBB.getTargetCFGNode(callStmt), 
																	 this.funcMap.getNewFuncGraphs(), 
																	 this.funcMap.getNewClassicCFG());
					retVal = checkUnlockPresent(newFuncBBs);
					
				}
				if(hasUnLock) {
					// get all the pre-dominator 
					// this is in new function
					Statement callStmt = (Statement)unlockCall.getParent();
					BasicBlock tarBB = this.funcMap.getNewClassicCFG().getBB(callStmt);
					Set<BasicBlock> newFuncBBs = getPreDominatorBBs(tarBB.getTargetCFGNode(callStmt), 
																	 this.funcMap.getNewFuncGraphs(), 
																	 this.funcMap.getNewClassicCFG());
					retVal = checkLockPresent(newFuncBBs);
				}
			}
		}
		
		return retVal;
	}

}

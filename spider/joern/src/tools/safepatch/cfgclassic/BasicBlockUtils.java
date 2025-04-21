/**
 * 
 */
package tools.safepatch.cfgclassic;

import java.util.ArrayList;
import java.util.List;

import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;
import ast.ASTNode;
import ast.expressions.CallExpression;
import ast.statements.ExpressionStatement;

/**
 * @author machiry
 *
 */
public class BasicBlockUtils {
	
	private static String[] TERMINATING_CALLS = {"panic", "die"};
	
	
	/***
	 * 
	 * @param targetASTNode
	 * @return
	 */
	public static boolean isTerminatingExpression(CFGNode targetASTNode) {
		boolean retVal = false;
		if(targetASTNode instanceof ASTNodeContainer) {
			ASTNodeContainer targetCont = (ASTNodeContainer)targetASTNode;
			ASTNode targetNode = targetCont.getASTNode();
			if(targetNode instanceof ExpressionStatement) {
				ExpressionStatement currSt = (ExpressionStatement)targetNode;
				if(currSt.getChildCount() > 0 && currSt.getChild(0) instanceof CallExpression) {
					CallExpression currCall = (CallExpression)currSt.getChild(0);
					for(String currS:BasicBlockUtils.TERMINATING_CALLS) {
						if(currCall.getEscapedCodeStr().contains(currS)) {
							retVal = true;
							break;
						}
					}
					
				}
			}
		}
		return retVal;		
	}
	
	private static boolean recursiveCheckPath(BasicBlock srcBlock, BasicBlock dstBlock, ArrayList<BasicBlock> visited) {
		if(srcBlock.equals(dstBlock)) {
			return true;
		}
		if(visited.contains(srcBlock)) {
			return false;
		}
		visited.add(srcBlock);
		for(BasicBlock childBB:srcBlock.getAllChildren()) {
			if(recursiveCheckPath(childBB, dstBlock, visited)) {
				return true;
			}
		}
		visited.remove(srcBlock);
		return false;
	}
	
	public static boolean isInLoop(BasicBlock currBB) {
		ArrayList<BasicBlock> visitedBB = new ArrayList<BasicBlock>();
		for(BasicBlock childBB:currBB.getAllChildren()) {
			if(BasicBlockUtils.recursiveCheckPath(childBB, currBB, visitedBB)) {
				return true;
			}
		}
		return false;
	}
	
	public static ArrayList<BasicBlock> getPathList(BasicBlock currBB, BasicBlock dstBB, List<BasicBlock> visited) {
		ArrayList<BasicBlock> toRet = new ArrayList<BasicBlock>();
		if(!visited.contains(currBB)) {
			visited.add(currBB);
			if(currBB.equals(dstBB)) {
				toRet.add(dstBB);
				return toRet;
			}
			ArrayList<BasicBlock> childBBPath = null;
			for(BasicBlock childBB:currBB.getAllChildren()) {
				childBBPath = getPathList(childBB, dstBB, visited);
				if(childBBPath.size() > 0) {
					toRet.add(currBB);
					toRet.addAll(childBBPath);
					return toRet;
				}
			}
			visited.remove(currBB);
		}
		return toRet;
	}
	
	public static String getFirstEdgeLabel(BasicBlock srcBB, BasicBlock dstBB) {
		ArrayList<BasicBlock> pathList = getPathList(srcBB, dstBB, new ArrayList<BasicBlock>());
		if(pathList.size() > 0) {
			BasicBlock firstCh = pathList.get(0);
			return srcBB.bbEdges.get(firstCh);
		}
		return null;
	}
}

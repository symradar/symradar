/**
 * 
 */
package tools.safepatch.cfgclassic;

import java.util.ArrayList;
import java.util.List;

import ast.ASTNode;
import ast.functionDef.FunctionDef;
import cdg.CDG;
import cdg.CDGEdge;
import cfg.CFGEdge;
import cfg.nodes.CFGNode;
import cfg.CFG;

/**
 * @author machiry
 *
 */
public class ClassicCFG {
	
	private static String fallThruEdgeLabel = "FALL";
	// list of all available BBs
	private ArrayList<BasicBlock> allBBs = new ArrayList<BasicBlock>();
	CFG joernCFG = null;
	FunctionDef joernDef = null;
	public String funcName = "";
	private long idCount = 0;
	
	public ClassicCFG(CFG joernCFGg, CDG joernCDG, FunctionDef joernFnDef) {
		this.joernCFG = joernCFGg;
		this.joernDef = joernFnDef;
		this.preProcessEdges(this.joernCFG.getEdges(), this.joernCFG);
		for(CFGEdge currEdge:this.joernCFG.getEdges()) {
			this.processEdge(currEdge);
		}
		this.funcName = this.joernDef.getLabel();
		for(BasicBlock currBB:this.allBBs) {
			currBB.getNumReachableStatments(null);
		}
		this.generateCDGraph(joernCDG);
	}
	
	public String getDigraphOutput() {
		String retString = "digraph " + this.joernDef.getFunctionSignature().split("\\(")[0].trim() + " {\n";
		// just add nodes
		for(BasicBlock currBB:this.allBBs) {
			retString += currBB.id +  " [label=\"" + currBB.getDotString() + "\"];\n";
		}
		// now add edges
		for(BasicBlock currBB:this.allBBs) {
			for(BasicBlock dstBB:currBB.bbEdges.keySet()) {
				retString += currBB.id + " -> " + dstBB.id + " [color=\""+ ClassicCFG.getEdgeColor(currBB.bbEdges.get(dstBB)) + "\", label=\"" + currBB.bbEdges.get(dstBB) + "\"];\n";
			}
		}
		retString += "}";
		return retString;
	}
	
	private synchronized long getID() {
		this.idCount++;
		return this.idCount;
	}
	
	private static String getEdgeColor(String edgeLabel) {
		if(edgeLabel == "True") {
			return "green";
		} 
		if(edgeLabel == "False") {
			return "red";
		}
		return "blue";
	}
	
	public ArrayList<BasicBlock> getAllBBs() {
		return this.allBBs;
	}
	
	/**
	 * 
	 */
	private void generateCDGraph(CDG targetCDCGraph) {
		for(CDGEdge currEd:targetCDCGraph.getEdges()){
			BasicBlock srcBB = this.getBB(currEd.getSource());
			BasicBlock dstBB = this.getBB(currEd.getDestination());
			// only of the src BB has more than 1 child
			if(srcBB.bbEdges.keySet().size() > 1) {
				dstBB.addCDBB(srcBB);
			}
		}
	}
	
	
	/***
	 * 
	 * @param targetCFGNode
	 * @return
	 */
	private BasicBlock getOrCreateBB(CFGNode targetCFGNode) {
		
		BasicBlock targetBB = null;
		for(BasicBlock currBB:this.allBBs) {
			if(currBB.containsNode(targetCFGNode)) {
				targetBB = currBB;
				break;
			}
		}
		if(targetBB == null) {
			targetBB = new BasicBlock(this.getID(), targetCFGNode, this.joernDef.getLocation().startLine);
			this.allBBs.add(targetBB);
		}
		return targetBB;
	}
	
	public BasicBlock getBB(CFGNode targetCFGNode) {
		BasicBlock targetBB = null;
		for(BasicBlock currBB:this.allBBs) {
			if(currBB.containsNode(targetCFGNode)) {
				targetBB = currBB;
				break;
			}
		}
		return targetBB;
	}
	
	public BasicBlock getBB(ASTNode targetASTNode) {
		BasicBlock targetBB = null;
		for(BasicBlock currBB:this.allBBs) {
			if(currBB.containsNode(targetASTNode)) {
				targetBB = currBB;
				break;
			}
		}
		return targetBB;
	}
	
	private void processEdge(CFGEdge currEdge) {
		CFGNode srcNode = currEdge.getSource();
		CFGNode dstNode = currEdge.getDestination();
		
		
		if(currEdge.getLabel() == CFGEdge.EMPTY_LABEL) {
			// sequential flow
			BasicBlock srcBB = this.getOrCreateBB(srcNode);
			BasicBlock dstBB = this.getBB(dstNode);
			if(dstBB != null) {
				srcBB.addChild(dstBB, ClassicCFG.fallThruEdgeLabel);
			} else {
				srcBB.addNode(dstNode);
			}
			
		} else {
			// if this is not empty then start a new BB for the destination.
			BasicBlock srcBB = this.getOrCreateBB(srcNode);
			
			BasicBlock dstBB = this.getOrCreateBB(dstNode);
			
			// sanity
			assert(srcBB != dstBB);
			srcBB.addChild(dstBB, currEdge.getLabel());
			
		}
		
	}
	
	private void preProcessEdges(List<CFGEdge> allEdges, CFG targetCFG) {
		// pre-process all the edges.		
		for(CFGEdge currEd:allEdges) {
			// every node which has multiple successors will be start of a BB.
			// every node which has multiple predecessors will also be a start of a BB.
			CFGNode srcNode = currEd.getSource();
			CFGNode dstNode = currEd.getDestination();
			
			if(targetCFG.inDegree(srcNode) > 1) {
				this.getOrCreateBB(srcNode);
			}
			if(targetCFG.outDegree(srcNode) > 1) {
				this.getOrCreateBB(srcNode);
			}
			if(targetCFG.inDegree(dstNode) > 1) {
				this.getOrCreateBB(dstNode);
			}
			if(targetCFG.outDegree(dstNode) > 1) {
				this.getOrCreateBB(dstNode);
			}			
		}
	}
}

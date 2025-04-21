package tools.safepatch.cfgimpl;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.jrubyparser.ast.HashNode;

import ast.functionDef.Parameter;
import ast.statements.Condition;
import cdg.CDG;
import cdg.CDGCreator;
import cfg.CFG;
import cfg.CFGEdge;
import cfg.C.CCFG;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGEntryNode;
import cfg.nodes.CFGErrorNode;
import cfg.nodes.CFGExceptionNode;
import cfg.nodes.CFGExitNode;
import cfg.nodes.CFGNode;

/**
 * @author Eric Camellini
 * CFG traversal module, it traverses every node only once, backward edges are not considered
 */
public class CFGIterator {

	private static final Logger logger = LogManager.getLogger();
	
	private CFG cfg;
	
	/* Node to which the walker points */
	private int currentNodeIndex;
	private int nodeCount;
	
	public int getCurrentNodeIndex(){
		return this.currentNodeIndex;
	}
	
	public CFGNode getCurrentNode() {
		if(this.currentNodeIndex >= this.nodeCount)
			return null;
		else return cfg.getVertices().get(currentNodeIndex);
	}

	public CFGIterator(CFG cfg) {
		this.cfg = cfg;
		this.initialize();
	}

	
	private void initialize() {
		this.currentNodeIndex = 0;
		this.nodeCount = this.cfg.getVertices().size();
		skipSpecialNodes();
	}
	
	private void skipSpecialNodes(){
		while(this.getCurrentNode() != null &&
				(this.getCurrentNode() instanceof CFGEntryNode ||
						this.getCurrentNode() instanceof CFGExitNode)){
			this.currentNodeIndex++;
		}

		if(this.cfg.getVertices().size() >= 2){
			if(this.cfg.getVertices().get(this.nodeCount - 1) instanceof CFGErrorNode ||
					this.cfg.getVertices().get(this.nodeCount - 2) instanceof CFGErrorNode){
				this.nodeCount--;
			}

			if(this.cfg.getVertices().get(this.nodeCount - 1) instanceof CFGExceptionNode ||
					this.cfg.getVertices().get(this.nodeCount - 2) instanceof CFGExceptionNode){
				this.nodeCount--;
			}
		}
	}
	
	/**
	 * Returns true if the CFG has more nodes to be visited
	 */
	public boolean hasNext(){
		return this.currentNodeIndex + 1 < this.nodeCount;
	}
	
	/**
	 * Moves the CFGWalker node pointer to the further node (only if hasNext() is true)
	 */
	public void moveToNext(){
		if(this.hasNext())
			this.currentNodeIndex++;
		else throw new RuntimeException("The CFG has no more nodes, moveToNext() failed.");
	}
	
	/**
	 * Returns the next node to be traversed in the CFG
	 */
	public CFGNode getNext(){
		if(this.hasNext())
			return this.cfg.getVertices().get(currentNodeIndex + 1);
		else
			return null;
	}
	
	/**
	 * Traverser all the nodes of the CFG without changing the state of the CFGWalker:
	 * the current node will be the same after calling this method.
	 */
//	public void traverseAll(){
//		for(CFGNode node : this.cfg.getVertices())
//			handleNode(node);
//	}
	
	
//	public void handleCurrentNode(){
//		if(logger.isDebugEnabled()) logger.debug(this.cfg.getVertices().get(currentNodePointer));
//	}

	
	
//	public void handleNode(CFGNode node){
//		if(logger.isDebugEnabled()) logger.debug(node);
//	}
	
	/**
	 * Visits the parameters of the function (first nodes in the CFG) and
	 * moved the node pointer on the first node after them.
	 */
//	public void visitParameters(){
//		 List<CFGEdge> outgoing = cfg.outgoingEdges(cfg.getEntryNode());
//		 CFGNode node = null;
//		 if(outgoing.size() == 1)
//			 node = outgoing.get(0).getDestination();
//		 while(node != null &&
//				 node instanceof ASTNodeContainer &&
//				 ((ASTNodeContainer) node).getASTNode() instanceof Parameter){
//			 
//			 moveToNext();
//			 handleCurrentNode();
//			 outgoing = cfg.outgoingEdges(node);
//			 node = null;
//			 if(outgoing != null && outgoing.size() == 1)
//				 node = outgoing.get(0).getDestination();
//		 }
//	}
}

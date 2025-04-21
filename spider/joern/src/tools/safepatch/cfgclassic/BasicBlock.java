/**
 * 
 */
package tools.safepatch.cfgclassic;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import ast.ASTNode;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;
import tools.safepatch.flowres.FlowRestrictiveMainConfig;

/**
 * @author machiry
 *
 */
public class BasicBlock {
	private CFGNode targetNode = null;
	public long id = -1;
	public ArrayList<CFGNode> allNodes = new ArrayList<CFGNode>();
	private boolean reachStatementsComputed = false;
	
	private long reachableStatments = 0;
	private boolean isTerminating = false;
	private long sourceLocation = 0;
	
	// for all the successor basicblocks, this contains a map between the child basicblock
	// and a string representing the edge type.
	public HashMap<BasicBlock, String> bbEdges = new HashMap<BasicBlock, String>();
	
	public ArrayList<BasicBlock> cdBBs = new ArrayList<BasicBlock>();
	
	public HashSet<BasicBlock> errorHandlingChildren = new HashSet<BasicBlock>();
	
	// yes there could be multiple parents.
	private ArrayList<BasicBlock> parents = new ArrayList<BasicBlock>();
	
	private boolean isErrorHandling = false;
	
	private boolean isErrorGuarding = false;
	
	public BasicBlock(long uniqueid, CFGNode targetNode, long funcOffset) {
		// TODO Auto-generated constructor stub
		this.id = uniqueid;
		this.targetNode = targetNode;
		this.isTerminating = BasicBlockUtils.isTerminatingExpression(targetNode);
		this.allNodes.add(targetNode);
		if(this.targetNode instanceof ASTNodeContainer) {
			ASTNodeContainer targetCont = (ASTNodeContainer)this.targetNode;
			ASTNode currANode = targetCont.getASTNode();
			this.sourceLocation  = currANode.getLocation().startLine + funcOffset + 1; 
		}
	}
	
	public long getStartLine() {
		return this.sourceLocation;
	}
	
	public Set<BasicBlock> getAllChildren() {
		return this.bbEdges.keySet();
	}
	
	public List<ASTNode> getAllASTNodes() {
		ArrayList<ASTNode> toRet = new ArrayList<ASTNode>();
		for(CFGNode currN:this.allNodes) {
			if(currN instanceof ASTNodeContainer) {
				toRet.add(((ASTNodeContainer)currN).getASTNode());
			}
		}
		return toRet;
	}
	
	public boolean containsNode(CFGNode currNode) {
		return this.allNodes.contains(currNode);
	}
	
	public boolean isTrueErrorHandling() {
		return this.isErrorHandling;
	}
	
	public boolean isConfigErrorHandling() {
		return this.isErrorHandling && !FlowRestrictiveMainConfig.ignoreErrorBB;
	}
	
	public void setErrorHandling(boolean errH) {
		this.isErrorHandling = errH;
	}
	
	public boolean isTrueErrorGuarding() {
		return this.isErrorGuarding;
	}
	
	public boolean isConfigErrorGuarding() {
		return this.isErrorGuarding && !FlowRestrictiveMainConfig.ignoreErrorBB;
	}
	
	public boolean isPositiveErrorGuarding() {
		if(this.isTrueErrorGuarding()) {
			BasicBlock errBB = (BasicBlock)this.errorHandlingChildren.toArray()[0];
			String edgeLabel = this.bbEdges.get(errBB).toLowerCase().trim();
			return edgeLabel.equals("false");
		}
		return false;
	}
	
	public boolean isNegativeErrorGuarding() {
		if(this.isTrueErrorGuarding()) {
			BasicBlock errBB = (BasicBlock)this.errorHandlingChildren.toArray()[0];
			String edgeLabel = this.bbEdges.get(errBB).toLowerCase().trim();
			return edgeLabel.equals("true");
		}
		return false;
	}
	
	public void setErrorGuarding(boolean errG) {
		this.isErrorGuarding = errG;
	}
	
	public ASTNode getLastNode() {
		ASTNode toRet = null;
		CFGNode lastNode = this.allNodes.get(this.allNodes.size() -1);
		if(lastNode instanceof ASTNodeContainer) {
			toRet = ((ASTNodeContainer)lastNode).getASTNode();
		}
		return toRet;
	}
	
	public void addErrorHandlingChild(BasicBlock errBB) {
		if(errBB != null) {
			this.errorHandlingChildren.add(errBB);
		}
	}
	
	public boolean containsNode(ASTNode currNode) {
		for(CFGNode cf:this.allNodes) {
			if(cf instanceof ASTNodeContainer) {
				ASTNodeContainer cont = (ASTNodeContainer)cf;
				if(cont.getASTNode() == currNode) {
					return true;
				}
			}
		}
		return false;
	}
	
	public CFGNode getTargetCFGNode(ASTNode currNode) {
		for(CFGNode cf:this.allNodes) {
			if(cf instanceof ASTNodeContainer) {
				ASTNodeContainer cont = (ASTNodeContainer)cf;
				if(cont.getASTNode() == currNode) {
					return cf;
				}
			}
		}
		return null;
	}
	
	public void addCDBB(BasicBlock srcBB) {
		if(!this.cdBBs.contains(srcBB)) {
			this.cdBBs.add(srcBB);
		}
	}
	
	public void addNode(CFGNode currNode) {
		if(!this.containsNode(currNode)){
			this.allNodes.add(currNode);
			this.isTerminating = BasicBlockUtils.isTerminatingExpression(currNode);
		}
	}
	
	public long getNumReachableStatments(HashSet<Long> visitedBBs) {
		if(visitedBBs != null && visitedBBs.contains(this.id)) {
			return 0;
		}
		if(this.reachStatementsComputed) {
			return reachableStatments;
		}
		
		this.reachableStatments = this.allNodes.size();
		// compute the children, only if the current BB is not terminating.
		if(!this.isTerminating) {
			if(visitedBBs == null) {
				visitedBBs = new HashSet<Long>();
			}
			visitedBBs.add(this.id);
			this.reachableStatments += this.getAllChildrenReachableStatements(visitedBBs);
			visitedBBs.remove(this.id);
		}
		this.reachStatementsComputed = true;
		return this.reachableStatments;
	}
	
	private long getAllChildrenReachableStatements(HashSet<Long> visited) {
		long retVal = 0;
		for(BasicBlock currBB:this.bbEdges.keySet()) {
			retVal += currBB.getNumReachableStatments(visited);
		}
		return retVal;
	}
	
	public void addChild(BasicBlock childBB, String targetEdge) {
		if(!bbEdges.containsKey(childBB)) {
			this.bbEdges.put(childBB, targetEdge);
			if(!childBB.parents.contains(this)) {
				childBB.parents.add(this);
			}
		}
	}
	
	public void propagateErrorHandling(List<BasicBlock> visited) {		
		if(!visited.contains(this)) {
			boolean newFlag = this.parents.size() > 0;
			for(BasicBlock currBB:this.parents) {
				newFlag = newFlag && currBB.isTrueErrorHandling();
			}
			this.isErrorHandling = this.isErrorHandling || newFlag;
			visited.add(this);
			for(BasicBlock currBB:this.bbEdges.keySet()) {
				currBB.propagateErrorHandling(visited);
			}
			visited.remove(this);
		}
	}
	
	public void propagateErrorHandling() {
		ArrayList<BasicBlock> visited = new ArrayList<BasicBlock>();
		this.propagateErrorHandling(visited);
	}
	
	
	@Override
	public int hashCode() {
		return this.targetNode.hashCode();
	}
	
	@Override
	public boolean equals(Object o) {
		if(o == this) {
			return true;
		}
		if(!(o instanceof BasicBlock)) {
			return false;
		}
		if(o instanceof CFGNode) {
			return this.targetNode.equals(o);
		}
		return this.targetNode.equals(((BasicBlock)o).targetNode);
	}
	
	@Override
	public String toString() {
		String toRet = "";
		for(CFGNode currN:this.allNodes) {
			toRet += currN.toString() + "\n";
		}
		return toRet;
	}
	
	public String getDotString() {
		String toRet = "";
		for(CFGNode currN:this.allNodes) {
			toRet += currN.toString() + "\\n";
		}
		return toRet;
	}

}

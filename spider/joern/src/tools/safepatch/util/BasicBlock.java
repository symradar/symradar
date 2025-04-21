package tools.safepatch.util;

import java.util.ArrayList;
import java.util.List;

import ast.ASTNode;
import cfg.nodes.ASTNodeContainer;

public class BasicBlock {
	
	private List<ASTNodeContainer> blockNodes;
	private List<ASTNode> blockStatements = null;
	
	public BasicBlock() {
		this.blockNodes = new ArrayList<ASTNodeContainer>();
	}
	
	public void addNode(ASTNodeContainer node){
		this.blockNodes.add(node);
	}
	
	public List<ASTNodeContainer> getBlockNodes(){
		return this.blockNodes;
	}
	
	/**
	 * Returns the list of Statements in the basic block.
	 * Since a basic block contains ASTNodeContainer objects
	 * extracted from a CCFG, not all the corresponding AstNodes
	 * are Statement object (e.g. there can be Condition object
	 * for branching nodes).
	 */
	public List<ASTNode> getBlockStatements(){
		if(this.blockStatements == null)
			extractBlockStatements();
		
		return this.blockStatements;
	}
	
	private void extractBlockStatements() {
		this.blockStatements = new ArrayList<ASTNode>();
		for(ASTNodeContainer node : this.blockNodes){
			this.blockStatements.add(node.getASTNode());
		}
	}

	@Override
	public String toString() {
		String retval = "\n";
		for(ASTNodeContainer node : this.blockNodes){
			retval += node.toString() + "\n";
		}
		return retval;
	}
}

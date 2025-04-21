package tools.safepatch.cfgimpl;

import cfg.nodes.ASTNodeContainer;

public class IfStackObject extends StackObject{

	private ASTNodeContainer thenNode;
	private ASTNodeContainer elseNode;
	

	public IfStackObject(ASTNodeContainer conditionNode,
			ASTNodeContainer thenNode, ASTNodeContainer elseNode) {
		super(conditionNode);
		this.thenNode = thenNode;
		this.elseNode = elseNode;
	}


	public ASTNodeContainer getThenNode() {
		return thenNode;
	}


	public ASTNodeContainer getElseNode() {
		return elseNode;
	}
	
	
}

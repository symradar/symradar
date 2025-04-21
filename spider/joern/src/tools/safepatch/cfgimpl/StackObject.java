package tools.safepatch.cfgimpl;

import cfg.nodes.ASTNodeContainer;

public class StackObject {
	
	protected ASTNodeContainer conditionNode;
	
	public StackObject(ASTNodeContainer conditionNode) {
		this.conditionNode = conditionNode;
	}

	public ASTNodeContainer getCondition(){
		return this.conditionNode;
	}
	
}

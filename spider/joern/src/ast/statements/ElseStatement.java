package ast.statements;

public class ElseStatement extends BlockStarter
{
	private IfStatement ifNode;

	public IfStatement getIfNode() {
		return ifNode;
	}

	public void setIfNode(IfStatement ifNode) {
		this.ifNode = ifNode;
	}
	
}

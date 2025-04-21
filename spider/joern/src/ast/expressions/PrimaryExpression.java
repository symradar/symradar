package ast.expressions;

import ast.walking.ASTNodeVisitor;

public class PrimaryExpression extends PostfixExpression
{
	public void accept(ASTNodeVisitor visitor)
	{
		visitor.visit(this);
	}
	
	@Override
	public String getLabel() {
		// TODO Auto-generated method stub
		return getEscapedCodeStr();
	}
}

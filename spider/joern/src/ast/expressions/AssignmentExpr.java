package ast.expressions;

import ast.ASTNode;
import ast.walking.ASTNodeVisitor;

public class AssignmentExpr extends BinaryExpression
{
	// TODO: This can probably be removed
	public void accept(ASTNodeVisitor visitor)
	{
		visitor.visit(this);
	}
	
	/**
	 * Returns the rightmost non-assignment node, the value assigned.
	 * Examples:
	 * a = b --> returns b;
	 * a = b = 0 --> returns 0;
	 */
	public ASTNode getAssignedValue(){
		if (this.getRight() instanceof AssignmentExpr) return ((AssignmentExpr) this.getRight()).getAssignedValue();
		else return this.getRight();
	}
}

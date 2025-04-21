package udg.symbols;

import java.util.List;

import ast.ASTNode;
import ast.expressions.Identifier;

public abstract class UseOrDefSymbol {

	private ASTNode node;
	
	public UseOrDefSymbol() {
	}
	
	public ASTNode getNode() {
		return node;
	}
	
	protected void setNode(ASTNode node){
		this.node = node;
	}
	
	public abstract String getSymbolValue();
	public abstract List<Identifier> getIdentifiers();
	
	public UnaryOperatorSymbol sorroundWithUnaryOperator(ASTNode n, String unaryOperatorValue){
		return new UnaryOperatorSymbol(n, this, unaryOperatorValue);
	}
	
	@Override
	public String toString() {
		return super.toString() + "_value[" + getSymbolValue() + "]";
		//return this.getClass().getSimpleName() + "@" + this.hashCode() + "_value[" + getSymbolValue() + "]";
	}
}

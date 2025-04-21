package udg.symbols;

import java.util.List;

import ast.ASTNode;
import ast.expressions.Identifier;

public class UnaryOperatorSymbol extends UseOrDefSymbol {
	
	/*
	 * This will be a UnaryOperator or PtrMemberAccess
	 */
	private UseOrDefSymbol childSymbol;
	private String operatorValue = "";

	
	public UnaryOperatorSymbol(ASTNode node, UseOrDefSymbol childSymbol, String operatorValue) {
		super();
		this.setNode(node);
		this.childSymbol = childSymbol;
		this.operatorValue = operatorValue;
	}


	@Override
	public String getSymbolValue() {
		return this.operatorValue + " " + this.childSymbol.getSymbolValue();
	}

	
	public UseOrDefSymbol getNestedUseOrDefSymbol() {
		return childSymbol;
	}


	public String getOperatorValue() {
		return operatorValue;
	}


	@Override
	public List<Identifier> getIdentifiers() {
		if(childSymbol != null){
			return childSymbol.getIdentifiers();
		} else
			throw new RuntimeException("UnaryOperatorSymbol without nested Symbol");

	}
	
}

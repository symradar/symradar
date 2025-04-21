package udg.symbols;

import java.util.ArrayList;
import java.util.List;

import ast.ASTNode;
import ast.expressions.Identifier;

public class CodeStringSymbol extends UseOrDefSymbol{
	
	private String codeString;
	private List<UseOrDefSymbol> childSymbols;
	
	public CodeStringSymbol(String codeString, ASTNode node) {
		super();
		this.codeString = codeString;
		this.setNode(node);
		this.childSymbols = new ArrayList<UseOrDefSymbol>();
	}

	public void addChildSymbol(UseOrDefSymbol symbol){
		this.childSymbols.add(symbol);
	}
	
	public void addChildSymbols(List<UseOrDefSymbol> symbols){
		this.childSymbols.addAll(symbols);
	}
	
	public String getCodeString() {
		return codeString;
	}

	@Override
	public String getSymbolValue() {
		return codeString;
	}

	@Override
	public List<Identifier> getIdentifiers() {
		List<Identifier> l = new ArrayList<Identifier>();
		for (UseOrDefSymbol s : childSymbols)
			l.addAll(s.getIdentifiers());
		return l;
	}
	
	
}

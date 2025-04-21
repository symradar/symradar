package udg.symbols;

import java.util.ArrayList;
import java.util.List;

import ast.ASTNode;
import ast.expressions.Identifier;

public class IdentifierSymbol extends UseOrDefSymbol {

	
	public IdentifierSymbol(Identifier identifier) {
		super();
		this.setNode(identifier);
	}

	@Override
	protected void setNode(ASTNode node) {
		if(!(node instanceof Identifier))
			throw new RuntimeException("Trying to create an IdentifierSymbol starting from a non-identifier node");
		super.setNode(node);
	}
	
	
	@Override
	public String getSymbolValue(){
		return this.getIdentifier().getEscapedCodeStr();
	}

	public Identifier getIdentifier(){
		return (Identifier) this.getNode();
	}
	
	@Override
	public List<Identifier> getIdentifiers() {
		List<Identifier> l = new ArrayList<Identifier>();
		l.add(this.getIdentifier());
		return l;
	}
	
}

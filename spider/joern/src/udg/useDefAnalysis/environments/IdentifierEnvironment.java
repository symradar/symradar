package udg.useDefAnalysis.environments;

import java.util.LinkedList;

import ast.expressions.Identifier;
import udg.ASTNodeASTProvider;
import udg.symbols.IdentifierSymbol;
import udg.symbols.UseOrDefSymbol;

public class IdentifierEnvironment extends UseDefEnvironment
{

	// pass the identifier up-stream as a symbol
	
	public LinkedList<UseOrDefSymbol> upstreamSymbols()
	{
		//String code = astProvider.getEscapedCodeStr();
		LinkedList<UseOrDefSymbol> retval = new LinkedList<UseOrDefSymbol>();
		Identifier id = (Identifier) ((ASTNodeASTProvider) astProvider).getASTNode();
		retval.add(new IdentifierSymbol(id));
		return retval;
	}
}

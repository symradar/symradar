package udg.useDefAnalysis.environments;

import java.util.Collection;
import java.util.LinkedList;

import udg.ASTNodeASTProvider;
import udg.ASTProvider;
import udg.symbols.CodeStringSymbol;
import udg.symbols.UseOrDefSymbol;
import udg.useDefGraph.UseOrDef;

public class MemberAccessEnvironment extends UseDefEnvironment
{

	private LinkedList<UseOrDefSymbol> allSymbols = new LinkedList<UseOrDefSymbol>();
	
	public LinkedList<UseOrDefSymbol> upstreamSymbols()
	{

		LinkedList<UseOrDefSymbol> retval = new LinkedList<UseOrDefSymbol>();
		
		// emit all symbols
		retval.addAll(symbols);

		// emit entire code string
		String codeStr = astProvider.getEscapedCodeStr();
		CodeStringSymbol c = new CodeStringSymbol(codeStr,
				((ASTNodeASTProvider) astProvider).getASTNode());
		c.addChildSymbols(allSymbols);
		retval.add(c);

		return retval;
	}

	public void addChildSymbols(LinkedList<UseOrDefSymbol> childSymbols,
			ASTProvider child)
	{
		int childNum = child.getChildNumber();
		// Only add the left child but never the right child
		if (childNum == 0)
			super.addChildSymbols(childSymbols, child);
		
		allSymbols.addAll(childSymbols);
	}

	public Collection<UseOrDef> useOrDefsFromSymbols(ASTProvider child)
	{
		return createUsesForAllSymbols(symbols);
	}

}

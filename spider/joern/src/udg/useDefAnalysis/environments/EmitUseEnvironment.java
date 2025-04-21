package udg.useDefAnalysis.environments;

import java.util.Collection;
import java.util.LinkedList;

import udg.ASTProvider;
import udg.symbols.UseOrDefSymbol;
import udg.useDefGraph.UseOrDef;

// emit all symbols as USE and don't hand
// anything to up stream.

public class EmitUseEnvironment extends UseDefEnvironment
{

	Collection<UseOrDefSymbol> useSymbols = new LinkedList<UseOrDefSymbol>();

	public void addChildSymbols(LinkedList<UseOrDefSymbol> childSymbols,
			ASTProvider child)
	{
		useSymbols.addAll(childSymbols);
	}

	public LinkedList<UseOrDefSymbol> upstreamSymbols()
	{
		// empty, unless a child-class adds something
		return symbols;
	}

	public Collection<UseOrDef> useOrDefsFromSymbols(ASTProvider child)
	{
		LinkedList<UseOrDef> retval = createUsesForAllSymbols(useSymbols);
		return retval;
	}
}

package udg.useDefAnalysis.environments;

import java.util.Collection;
import java.util.LinkedList;

import udg.ASTProvider;
import udg.symbols.UseOrDefSymbol;
import udg.useDefGraph.UseOrDef;

public class EmitDefAndUseEnvironment extends UseDefEnvironment
{

	Collection<UseOrDefSymbol> defSymbols = new LinkedList<UseOrDefSymbol>();
	Collection<UseOrDefSymbol> useSymbols = new LinkedList<UseOrDefSymbol>();

	public void addChildSymbols(LinkedList<UseOrDefSymbol> childSymbols,
			ASTProvider child)
	{
		if (isDef(child))
			defSymbols.addAll(childSymbols);
		if (isUse(child))
			useSymbols.addAll(childSymbols);
	}

	public Collection<UseOrDef> useOrDefsFromSymbols(ASTProvider child)
	{
		LinkedList<UseOrDef> retval = new LinkedList<UseOrDef>();

		if (isDef(child))
			retval.addAll(createDefsForAllSymbols(defSymbols));

		if (isUse(child))
			retval.addAll(createUsesForAllSymbols(useSymbols));

		return retval;
	}

	public LinkedList<UseOrDefSymbol> upstreamSymbols()
	{
		return emptySymbolList;
	}

}

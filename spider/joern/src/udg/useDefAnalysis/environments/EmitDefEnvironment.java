package udg.useDefAnalysis.environments;

import java.util.Collection;
import java.util.LinkedList;

import udg.ASTProvider;
import udg.symbols.UseOrDefSymbol;
import udg.useDefGraph.UseOrDef;

public class EmitDefEnvironment extends UseDefEnvironment
{

	Collection<UseOrDefSymbol> defSymbols = new LinkedList<UseOrDefSymbol>();

	public void addChildSymbols(LinkedList<UseOrDefSymbol> childSymbols,
			ASTProvider child)
	{
		if (isDef(child))
		{
			defSymbols.addAll(childSymbols);
		}
		if (isUse(child))
			symbols.addAll(childSymbols);
	}

	public Collection<UseOrDef> useOrDefsFromSymbols(ASTProvider child)
	{
		LinkedList<UseOrDef> retval = createDefsForAllSymbols(defSymbols);
		retval.addAll(createUsesForAllSymbols(symbols));
		return retval;
	}

}

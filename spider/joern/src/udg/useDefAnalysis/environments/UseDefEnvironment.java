package udg.useDefAnalysis.environments;

import java.util.Collection;
import java.util.LinkedList;

import udg.ASTProvider;
import udg.symbols.UseOrDefSymbol;
import udg.useDefGraph.UseOrDef;

/**
 * Base-class and default implementation of UseDefEnvironment.
 * */

public class UseDefEnvironment
{

	protected ASTProvider astProvider;
	LinkedList<UseOrDefSymbol> symbols = new LinkedList<UseOrDefSymbol>();

	static final LinkedList<UseOrDef> emptyUseOrDef = new LinkedList<UseOrDef>();
	static final LinkedList<UseOrDefSymbol> emptySymbolList = new LinkedList<UseOrDefSymbol>();

	public boolean isUse(ASTProvider child)
	{
		return false;
	}

	public boolean isDef(ASTProvider child)
	{
		return false;
	}

	public boolean shouldTraverse(ASTProvider child)
	{
		return true;
	}

	public void setASTProvider(ASTProvider anASTProvider)
	{
		this.astProvider = anASTProvider;
	}

	public ASTProvider getASTProvider()
	{
		return astProvider;
	}

	/**
	 * Propagate all symbols to upstream
	 * */

	public LinkedList<UseOrDefSymbol> upstreamSymbols()
	{
		return symbols;
	}

	/**
	 * Add all given symbols without exception
	 **/

	public void addChildSymbols(LinkedList<UseOrDefSymbol> childSymbols,
			ASTProvider child)
	{
		symbols.addAll(childSymbols);
	}

	/**
	 * By default, don't generate any uses or defs for symbols.
	 * */

	public Collection<UseOrDef> useOrDefsFromSymbols(ASTProvider child)
	{
		return emptyUseOrDef;
	}

	/* Utilities below */

	protected LinkedList<UseOrDef> createDefsForAllSymbols(
			Collection<UseOrDefSymbol> symbols)
	{
		return createDefOrUseForSymbols(symbols, true);
	}

	protected LinkedList<UseOrDef> createUsesForAllSymbols(
			Collection<UseOrDefSymbol> symbols)
	{
		return createDefOrUseForSymbols(symbols, false);
	}

	private LinkedList<UseOrDef> createDefOrUseForSymbols(
			Collection<UseOrDefSymbol> symbols, boolean isDef)
	{
		LinkedList<UseOrDef> retval = new LinkedList<UseOrDef>();
		for (UseOrDefSymbol s : symbols)
		{
			UseOrDef useOrDef = new UseOrDef();
			useOrDef.isDef = isDef;
			useOrDef.symbol = s;
			useOrDef.astProvider = this.astProvider;
			retval.add(useOrDef);
		}
		return retval;
	}

};
package ddg.DefUseCFG;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import misc.MultiHashMap;
import udg.symbols.IdentifierSymbol;
import udg.symbols.UnaryOperatorSymbol;
import udg.symbols.UseOrDefSymbol;

/**
 * A CFG decorated with USE and DEFs suitable to determine reaching definitions.
 * This contains the same information as a UseDefGraph (UDG) but in a format
 * better suited for DDG creation.
 * */

public class DefUseCFG
{

	private LinkedList<Object> statements = new LinkedList<Object>();
	private MultiHashMap<Object, Object> symbolsUsed = new MultiHashMap<Object, Object>();
	private MultiHashMap<Object, Object> identifierSymbolsUsed = new MultiHashMap<Object, Object>();
	private MultiHashMap<Object, Object> symbolsDefined = new MultiHashMap<Object, Object>();
	private MultiHashMap<Object, Object> parentBlocks = new MultiHashMap<Object, Object>();
	private MultiHashMap<Object, Object> childBlocks = new MultiHashMap<Object, Object>();
	private Map<String, Object> symbolIds = new HashMap<String, Object>();
	
	private Object exitNode;
	private List<String> parameters = new LinkedList<String>();
	
	private static final List<Object> EMPTY_LIST = new LinkedList<Object>();

	public void addStatement(Object statementId)
	{
		statements.add(statementId);
	}

	public void addSymbolUsed(Object key, String symbol)
	{
		symbolsUsed.add(key, symbol);
		identifierSymbolsUsed.add(key, symbol);
	}

	public void addSymbolDefined(Object key, String symbol)
	{
		symbolsDefined.add(key, symbol);
	}

	public Collection<Object> getSymbolsDefinedBy(Object blockId)
	{
		List<Object> listForKey = symbolsDefined.get(blockId);
		if (listForKey == null)
			return EMPTY_LIST;
		
		return listForKey;
	}
	
	/*private Collection<Object> removeUnnecessaryDefs(Collection<Object> originalColl) {
		if(originalColl.size() > 1) {
			List<String> allDefs = new ArrayList<String>();
			List<String> dupeDefs = new ArrayList<String>();
			// first convert the collection into list of strings
			for(Object currSt:originalColl) {
				if(currSt instanceof String) {
					String currV = (String)currSt;
					currV = currV.replaceAll("\\s+","");
					if(currV.contains(".")) {
						currV.split(".")
					}
				}
			}
		}
		return originalColl;
	}*/
	
	public Collection<Object> getSymbolsUsedBy(Object blockId)
	{
		List<Object> listForKey = symbolsUsed.get(blockId);
		if (listForKey == null)
			return EMPTY_LIST;
		return listForKey;
	}
	
	public Collection<Object> getIdentifierSymbolsUsedBy(Object blockId)
	{
		List<Object> listForKey = identifierSymbolsUsed.get(blockId);
		if (listForKey == null)
			return EMPTY_LIST;
		return listForKey;
	}
	
	public void addParentBlock(Object thisBlockId, Object parentId)
	{
		parentBlocks.add(thisBlockId, parentId);
	}

	public void addChildBlock(Object thisBlockId, Object childId)
	{
		childBlocks.add(thisBlockId, childId);
	}

	public LinkedList<Object> getStatements()
	{
		return statements;
	}

	public MultiHashMap<Object, Object> getSymbolsUsed()
	{
		return symbolsUsed;
	}

	public MultiHashMap<Object, Object> getSymbolsDefined()
	{
		return symbolsDefined;
	}

	public MultiHashMap<Object, Object> getParentBlocks()
	{
		return parentBlocks;
	}

	public MultiHashMap<Object, Object> getChildBlocks()
	{
		return childBlocks;
	}

	public Object getIdForSymbol(UseOrDefSymbol symbol)
	{
		return symbolIds.get(symbol);
	}

	public void setSetSymbolId(String symbolCode, Object symbolId)
	{
		symbolIds.put(symbolCode, symbolId);
	}

	public void addUsesForExitNode()
	{

		for (String symbol : parameters)
		{
			// this.addSymbolUsed(exitNode, symbol);
			this.addSymbolUsed(exitNode, "* " + symbol);
		}	
	}

	public void setExitNode(Object exitNode)
	{
		this.exitNode = exitNode;
	}

	public void setParameters(LinkedList<String> params)
	{
		parameters = params;
	}

	public void addParameter(String str)
	{
		parameters.add(str);
	}
	
	private boolean isInterestingSymbol(UseOrDefSymbol symbol) {
		return (symbol instanceof IdentifierSymbol) || (symbol instanceof UnaryOperatorSymbol);
	}
	
	
	public void addSymbolUsed(Object key, UseOrDefSymbol symbol)
	{
		if(this.isInterestingSymbol(symbol)) {
			identifierSymbolsUsed.add(key, symbol);
		}
		symbolsUsed.add(key, symbol);
	}

	public void addSymbolDefined(Object key, UseOrDefSymbol symbol)
	{
		symbolsDefined.add(key, symbol);
	}
}

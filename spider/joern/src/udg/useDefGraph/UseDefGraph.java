package udg.useDefGraph;

import java.util.ArrayList;
import java.util.List;

import misc.MultiHashMap;
import udg.symbols.UseOrDefSymbol;
import ast.ASTNode;

public class UseDefGraph
{

	/*
	 * TODO
	 * - Find a way to get by identifier
	 * 
	 */
	
	// A UseDefGraph is a table indexed
	// by identifiers. Each table-entry
	// is a list of the UseOrDefRecords
	// of the identifier.

	MultiHashMap<String, UseOrDefRecord> useOrDefRecordTable = new MultiHashMap<String, UseOrDefRecord>();

	public MultiHashMap<String, UseOrDefRecord> getUseDefDict()
	{
		return useOrDefRecordTable;
	}

	public List<UseOrDefRecord> getUsesAndDefsForSymbol(String symbol)
	{
		return useOrDefRecordTable.get(symbol);
	}

//	public List<UseOrDefRecord> getUsesAndDefsForSymbol(String value)
//	{
//		List<UseOrDefRecord> output = new ArrayList<>();
//		for (UseOrDefSymbol s : useOrDefRecordTable.keySet()){
//			if (s.getSymbolValue().equals(value))
//				output.addAll(useOrDefRecordTable.get(s));
//		}
//		return output;
//	}
	
	public void addDefinition(String symbol, ASTNode astNode)
	{
		add(symbol, astNode, true);
	}

	public void addUse(String symbol, ASTNode astNode)
	{
		add(symbol, astNode, false);
	}

	private void add(String symbol, ASTNode astNode, boolean isDef)
	{
		UseOrDefRecord record = new UseOrDefRecord(astNode, isDef);
		useOrDefRecordTable.add(symbol, record);
	}

}

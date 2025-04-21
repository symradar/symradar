package tools.safepatch.rw;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import ast.ASTNode;
import ast.expressions.Identifier;
import udg.symbols.UseOrDefSymbol;

/**
 * @author Eric Camellini
 *
 */
public class ReadWriteTable {
	
	Map<ASTNode, List<UseOrDefSymbol>> readTable;
	Map<ASTNode, List<UseOrDefSymbol>> writeTable;
	
	
	public Map<ASTNode, List<UseOrDefSymbol>> getReadTable() {
		return readTable;
	}

	public Map<ASTNode, List<UseOrDefSymbol>> getWriteTable() {
		return writeTable;
	}

	public ReadWriteTable() {
		readTable = new HashMap<ASTNode, List<UseOrDefSymbol>>();
		writeTable = new HashMap<ASTNode, List<UseOrDefSymbol>>();
	}
	
	public void addRead(ASTNode node, UseOrDefSymbol symbol){
		if(readTable.get(node) == null)
			readTable.put(node, new ArrayList<UseOrDefSymbol>());
		
		readTable.get(node).add(symbol);
	}
	
	public void addWrite(ASTNode node, UseOrDefSymbol symbol){
		if(writeTable.get(node) == null)
			writeTable.put(node, new ArrayList<UseOrDefSymbol>());
		
		writeTable.get(node).add(symbol);
	}
	
	/*
	 * Returns the reads performed by the portion of the AST
	 * that has node as root. If no reads are performed
	 * an empty list is returned.
	 */
	public List<UseOrDefSymbol> getReads(ASTNode node){
		List<UseOrDefSymbol> out = this.readTable.get(node);
		return out != null ? out : new ArrayList<UseOrDefSymbol>();
	}
	
	/*
	 * Returns the writes performed by the portion of the AST
	 * that has node as root. If no writes are performed
	 * an empty list is returned.
	 */
	public List<UseOrDefSymbol> getWrites(ASTNode node){
		List<UseOrDefSymbol> out = this.writeTable.get(node);
		return out != null ? out : new ArrayList<UseOrDefSymbol>();
	}
	
	public Set<String> getReadSymbolValues(ASTNode node){
		return useOrDefSymbolsToValueSet(this.getReads(node));
	}
	
	public Set<String> getWriteSymbolValues(ASTNode node){
		return useOrDefSymbolsToValueSet(this.getWrites(node));
	}
	
	public Set<Identifier> getReadIdentifiers(ASTNode node){
		return useOrDefSymbolsToIdentifierSet(this.getReads(node));
	}
	
	public Set<Identifier> getWriteIdentifiers(ASTNode node){
		return useOrDefSymbolsToIdentifierSet(this.getWrites(node));
	}
	
	/*
	 * This function takes as input a List of UseOrDefSymbols,
	 * gets the value of every symbol in it and puts it in a set.
	 * In this way if there are two different symbols corresponding to
	 * the same value (e.g. two identifiers with the same value) the 
	 * value will appear only once in the list.
	 */
	public static Set<String> useOrDefSymbolsToValueSet(List<UseOrDefSymbol> symbols){
		Set<String> out = new HashSet<String>();
		for (UseOrDefSymbol s : symbols)
			out.add(s.getSymbolValue());
		return out;
	}

	/*
	 * This function takes as input a List of UseOrDefSymbols,
	 * gets the Identifier node forom every symbol in it and puts it in 
	 * the return value (a set).
	 */
	public static Set<Identifier> useOrDefSymbolsToIdentifierSet(List<UseOrDefSymbol> symbols){
		Set<Identifier> out = new HashSet<Identifier>();
		for (UseOrDefSymbol s : symbols)
			out.addAll(s.getIdentifiers());
		return out;
	}
}

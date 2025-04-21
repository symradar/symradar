package tools.safepatch.rw;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.github.gumtreediff.tree.ITree;

import ast.ASTNode;
import ast.expressions.Identifier;
import ast.statements.Statement;
import tools.safepatch.diff.ASTDelta;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.fgdiff.ASTDiff;
import tools.safepatch.fgdiff.ASTDiff.ACTION_TYPE;
import udg.symbols.UseOrDefSymbol;

/**
 * @author Eric Camellini
 *
 */
public class ReadWriteUtil {

	
	private static final Logger logger = LogManager.getLogger();
	
	private static interface ReadsOrWritesGetter {

		List<UseOrDefSymbol> get(ASTNode n, ReadWriteTable readWriteTable);
	}

	private static class ReadsGetter implements ReadsOrWritesGetter{
		@Override
		public List<UseOrDefSymbol> get(ASTNode n, ReadWriteTable readWriteTable) {
			return readWriteTable.getReads(n);
		}		
	}

	private static class WritesGetter implements ReadsOrWritesGetter{
		@Override
		public List<UseOrDefSymbol> get(ASTNode n, ReadWriteTable readWriteTable) {
			return readWriteTable.getWrites(n);
		}	
	}

	private static enum InsertedOrDeleted {
		INSERTED,
		DELETED;
	}

	public static List<UseOrDefSymbol> insertedReads(ASTDelta d, FunctionDiff f){
		return insertedOrDeletedReadsOrWrites(d, f, new ReadsGetter(), InsertedOrDeleted.INSERTED);
	}

	public static List<UseOrDefSymbol> deletedReads(ASTDelta d, FunctionDiff f){
		return insertedOrDeletedReadsOrWrites(d, f, new ReadsGetter(), InsertedOrDeleted.DELETED);
	}

	public static List<UseOrDefSymbol> insertedWrites(ASTDelta d, FunctionDiff f){
		return insertedOrDeletedReadsOrWrites(d, f, new WritesGetter(), InsertedOrDeleted.INSERTED);
	}

	public static List<UseOrDefSymbol> deletedWrites(ASTDelta d, FunctionDiff f){
		return insertedOrDeletedReadsOrWrites(d, f, new WritesGetter(), InsertedOrDeleted.DELETED);
	}

	/*
	 * This method can be used to extract inserted/deleted reads/writes.
	 * If d != null then the information is extracted for a single delta, otherwise
	 * for the whole function.
	 */
	private static List<UseOrDefSymbol> insertedOrDeletedReadsOrWrites(ASTDelta d, FunctionDiff f, ReadsOrWritesGetter getter, InsertedOrDeleted insertedOrDeleted){
		
		/*
		 * The RW table must be the correct one accordingly to the chosen AST statements 
		 */
		ReadWriteTable table = (insertedOrDeleted == InsertedOrDeleted.INSERTED ? 
				f.getNewReadWriteTable() :
					f.getOriginalReadWriteTable());

		String debug1 = (getter instanceof ReadsGetter ? "read" : "write");
		String debug2 = (insertedOrDeleted == InsertedOrDeleted.INSERTED ? "inserted" : "deleted");
		
		if(logger.isInfoEnabled()) logger.info("Extracting {} {}s for ASTDelta {}", debug2, debug1, d);

		List<UseOrDefSymbol> output = new ArrayList<UseOrDefSymbol>();

		if(d != null){
			
			if(!f.getAllDeltas().contains(d))
				throw new RuntimeException("The ASTDelta d must be a delta contained in the function f");
			//DELTA LEVEL
			/*
			 * Statements list filled with original/new statements taken from the delta
			 * depending on the INSERTED/DELETED parameter: if we want to find the inserted
			 * reads we have to analyze the new AST, otherwise the original one.
			 */ 
			List<Statement> statements = (insertedOrDeleted == InsertedOrDeleted.INSERTED ? 
						d.getNewStatements() : 
							d.getOriginalStatements());
			
			if(statements != null){
				for(Statement s : statements){
					/*
					 * For every Statement in the statements list we extract the reads/writes
					 * using the RW table
					 */
					output.addAll(
						insertedOrDeletedReadsOrWrites(s, table, d.getFineGrainedDiff(), getter, insertedOrDeleted)
						);
				}
			}
			
		} else {
			//FUNCTION LEVEL
			/*
			 * Statements list filled with original/new statements taken from the function
			 * depending on the INSERTED/DELETED parameter: if we want to find the inserted
			 * reads we have to analyze the new AST, otherwise the original one.
			 */ 
			List<ASTNode> statements = (insertedOrDeleted == InsertedOrDeleted.INSERTED ? 
						f.getNewFunction().getContent().getStatements() : 
							f.getOriginalFunction().getContent().getStatements());
			
			for(ASTNode s : statements){
				/*
				 * For every Statement in the statements list we extract the reads/writes
				 * using the RW table
				 */
				output.addAll(
					insertedOrDeletedReadsOrWrites(s, table, f.getFineGrainedDiff(), getter, insertedOrDeleted)
					);
			}
		}

		return output;
	}

	private static List<UseOrDefSymbol> insertedOrDeletedReadsOrWrites(ASTNode n, ReadWriteTable table, ASTDiff diff, 
			ReadsOrWritesGetter getter, InsertedOrDeleted insertedOrDeleted){
		
		String debug1 = (getter instanceof ReadsGetter ? "read" : "write");
		String debug2 = (insertedOrDeleted == InsertedOrDeleted.INSERTED ? "inserted" : "deleted");
		if(logger.isInfoEnabled()) logger.info("Extracting {} {}s for Statement {}", debug2, debug1, n);
		
		List<UseOrDefSymbol> symbols = getter.get(n, table);
		if(logger.isInfoEnabled()) logger.info("{}s:", debug1);
		if(logger.isInfoEnabled()) logger.info(symbols);
		
		List<UseOrDefSymbol> output = new ArrayList<UseOrDefSymbol>();
		if(symbols != null){
			for(UseOrDefSymbol symbol : symbols){
				/*
				 * For every read/write we get the corresponding identifier nodes and check,
				 * for each one of these, the related action in the AST diff.
				 * In general we care only if a node is inserted (if we look for inserted 
				 * reads/writes), deleted (if we look for deleted ones) or updated (in both cases).
				 * I consider a read/write UseOrDefSymbol as inserted/deleted if at least one
				 * of its child Identifiers is inserted/deleted/updated.
				 */
				for(Identifier i : symbol.getIdentifiers()){
					if(insertedOrDeleted == InsertedOrDeleted.INSERTED){
						//Looking for inserted reads/writes
						if (diff.getActionByNode(i) == ASTDiff.ACTION_TYPE.INSERT){
							if(!output.contains(symbol))
								output.add(symbol);
						}
					}

					if(insertedOrDeleted == InsertedOrDeleted.DELETED){
						//Looking for deleted reads/writes
						if (diff.getActionByNode(i) == ASTDiff.ACTION_TYPE.DELETE){
							if(!output.contains(symbol))
								output.add(symbol);
						}

					}

					//Updates must be considered in both cases
					if (diff.getActionByNode(i) == ASTDiff.ACTION_TYPE.UPDATE){
						//Let's check if the identifier node is actually updated
						//or if it is part of a bigger update and for some weird
						//Gumtree behaviour still considered as updated
						
						//This will never be null since: i is marked as updated
						//there must be a mapped node in the other tree
						Identifier mappedId = (Identifier) diff.getMappedNode(i);
						
						if(!output.contains(symbol) && 
								!mappedId.getEscapedCodeStr().equals(i.getEscapedCodeStr()))
							output.add(symbol);
					}
				}
			}
		}
		return output;

	}
	
	public static Set<String> readsWritesDifference(ASTNode oldNode, ASTNode newNode, 
			ReadWriteTable oldTable, ReadWriteTable newTable,
			ReadsOrWritesGetter getter, InsertedOrDeleted insertedOrDeleted){

		List<UseOrDefSymbol> oldReadsOrWrites = getter.get(oldNode, oldTable);
		List<UseOrDefSymbol> newReadsOrWrites = getter.get(newNode, newTable);


		Set<String> newReadsOrWritesValues = ReadWriteTable.useOrDefSymbolsToValueSet(newReadsOrWrites);
		Set<String> oldReadsOrWritesValues = ReadWriteTable.useOrDefSymbolsToValueSet(oldReadsOrWrites);

		if(insertedOrDeleted == InsertedOrDeleted.INSERTED){
			return com.google.common.collect.Sets.difference(newReadsOrWritesValues, oldReadsOrWritesValues);
		} else {
			return com.google.common.collect.Sets.difference(oldReadsOrWritesValues, newReadsOrWritesValues);
		}
	}
	
}

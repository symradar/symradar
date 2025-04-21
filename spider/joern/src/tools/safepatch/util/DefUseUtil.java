package tools.safepatch.util;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.github.gumtreediff.gen.php.PhpParser.instanceOf_return;
import com.github.gumtreediff.tree.ITree;

import ast.ASTNode;
import ast.expressions.Identifier;
import ast.statements.Statement;
import ddg.DefUseCFG.DefUseCFG;
import tools.safepatch.diff.ASTDelta;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.diff.ASTDelta.DELTA_TYPE;
import udg.symbols.UseOrDefSymbol;
import udg.useDefGraph.UseDefGraph;
import udg.useDefGraph.UseOrDefRecord;


/**
 * @deprecated NOT USED ANYMORE
 * 
 * @author Eric Camellini
 *
 */
public class DefUseUtil {

	/*
	 * TODO 
	 * - Insert method to study inserted/delete reads/writes at a function
	 * 		level without passing through the deltas. 
	 */


	private static final Logger logger = LogManager.getLogger();

	/*
	 * Returns a map containing writes (defs) performed by every Statement in the list 
	 * l and by every ASTNode that descends from it (every node has its own entry in the Map).
	 * Write information are extracted from the DefUseCFG g.
	 */
	public static Map<ASTNode, List<Object>> defs(List<Statement> l, UseDefGraph udg){
		return defsOrUses(l, udg, new DefsGetter());
	}

	/*
	 * Returns a map containing writes (defs) performed by the ASTNode n 
	 * and by every ASTNode that descends from it (every node has its own entry in the Map).
	 * Write information are extracted from the DefUseCFG g.
	 */
	public static Map<ASTNode, List<Object>> defs(ASTNode n, UseDefGraph udg){
		return defsOrUses(n, udg, new DefsGetter());
	}

	public static Map<ASTNode, List<Object>> defs(ASTNode n, DefUseCFG ducfg){
		return defsOrUses(n, ducfg, new DefsGetter());
	}
	
	/*
	 * Fills the Map defines with the writes (defs) performed by the ASTNode n 
	 * and by every ASTNode that descends from it (every node has its own entry in the Map).
	 * Write information are extracted from the DefUseCFG g.
	 */
	public static void	defs(ASTNode n, UseDefGraph udg, Map<ASTNode, List<Object>> defines){
		defsOrUses(n, udg, new DefsGetter(), defines);
	}

	/*
	 * Returns the writes (defs) that are inserted by the diff delta d and that were not
	 * already present in the old version of the file.
	 * Write information are extracted from the DefUseCFG of the new version of the function
	 * in the f FunctionDiff.
	 */
	public static Map<ASTNode, List<Object>> insertedDefs(ASTDelta d, FunctionDiff f){
		return insertedOrDeletedDefsOrUsesFineGrained(d, f, new DefsGetter(), InsertedOrDeleted.INSERTED);
	}

	/*
	 * Returns the writes (defs) that are deleted by the diff delta d (that are not present anymore
	 * in the new file).
	 * Write information are extracted from the DefUseCFG of the old version of the function
	 * in the f FunctionDiff.
	 */
	public static Map<ASTNode, List<Object>> deletedDefs(ASTDelta d, FunctionDiff f){
		return insertedOrDeletedDefsOrUsesFineGrained(d, f, new DefsGetter(), InsertedOrDeleted.DELETED);
	}

	/*
	 * Returns true if the diff delta d is a write change (i.e if it introduces new writes OR deletes
	 * some of them).
	 */
	public static boolean isWrite(ASTDelta d, FunctionDiff f){
		//return isWriteOrRead(d, f.getNewGraphs().getDefUseCfg(), new DefsGetter());
		return ((insertedDefs(d, f).size() != 0) ||
				(deletedDefs(d, f).size() != 0));
	}


	/*
	 * Returns a map containing reads (uses) performed by every Statement in the list 
	 * l and by every ASTNode that descends from it (every node has its own entry in the Map).
	 * Read information are extracted from the DefUseCFG g.
	 */
	public static Map<ASTNode, List<Object>> uses(List<Statement> l, UseDefGraph udg){
		return defsOrUses(l, udg, new UsesGetter());
	}

	/*
	 * Returns a map containing reads (uses) performed by the Statement s 
	 * and by every ASTNode that descends from it (every node has its own entry in the Map).
	 * Read information are extracted from the DefUseCFG g.
	 */
	public static Map<ASTNode, List<Object>> uses(ASTNode n, UseDefGraph udg){
		return defsOrUses(n, udg, new UsesGetter());
	}

	public static Map<ASTNode, List<Object>> uses(ASTNode n, DefUseCFG ducfg){
		return defsOrUses(n, ducfg, new UsesGetter());
	}
	
	/*
	 * Fills the Map uses with the reads (uses) performed by the ASTNode n 
	 * and by every ASTNode that descends from it (every node has its own entry in the Map).
	 * Read information are extracted from the DefUseCFG g.
	 */
	public static void	uses(ASTNode n, UseDefGraph udg, Map<ASTNode, List<Object>> uses){
		defsOrUses(n, udg, new UsesGetter(), uses);
	}

	/*
	 * Returns the reads (uses) that are inserted by the diff delta d and that were not
	 * already present in the old version of the file.
	 * Read information are extracted from the DefUseCFG of the new version of the function
	 * in the f FunctionDiff.
	 */
	public static Map<ASTNode, List<Object>> insertedUses(ASTDelta d, FunctionDiff f){
		return insertedOrDeletedDefsOrUsesFineGrained(d, f, new UsesGetter(), InsertedOrDeleted.INSERTED);
	}

	/*
	 * Returns the reads (uses) that are deleted by the diff delta d (that are not present anymore
	 * in the new file).
	 * Read information are extracted from the DefUseCFG of the old version of the function
	 * in the f FunctionDiff.
	 */
	public static Map<ASTNode, List<Object>> deletedUses(ASTDelta d, FunctionDiff f){
		return insertedOrDeletedDefsOrUsesFineGrained(d, f, new UsesGetter(), InsertedOrDeleted.DELETED);
	}

	/*
	 * Returns true if the diff delta d is a read change (i.e if it introduces new reads or
	 * deletes some of them).
	 */
	public static boolean isRead(ASTDelta d, FunctionDiff f){
		//return isWriteOrRead(d, f.getNewGraphs().getDefUseCfg(), new UsesGetter());
		return ((insertedUses(d, f).size() != 0) ||
				(deletedUses(d, f).size() != 0));
	}

	/*
	 * Returns true if the diff delta d is a read change AND not a write change.
	 */
	public static boolean isReadOnly(ASTDelta d, FunctionDiff f){
		return !isWrite(d, f) && isRead(d, f);
	}


	/* ----------------------------------------------------
	 * The stuff below is what is used in the methods above
	   ---------------------------------------------------- */

	private static interface DefsOrUsesGetter {
		public Collection<Object> getByNode(DefUseCFG g, ASTNode n);
		public List<ASTNode> getBySymbol(UseDefGraph udg, String symbol);
		public boolean test(UseOrDefRecord r);
	}

	private static class DefsGetter implements DefsOrUsesGetter{

		@Override
		public Collection<Object> getByNode(DefUseCFG g, ASTNode n) {
			return g.getSymbolsDefinedBy(n);
		}

		@Override
		public List<ASTNode> getBySymbol(UseDefGraph udg, String symbol) {
			List<ASTNode> nodes = new ArrayList<>();
			List<UseOrDefRecord> l = udg.getUsesAndDefsForSymbol(symbol);
			if(l != null){
				for(UseOrDefRecord r : l){
					if(r.isDef())
						nodes.add(r.getAstNode());

				}
				return nodes;
			}
			else
				return null;
		}

		@Override
		public boolean test(UseOrDefRecord r) {
			return r.isDef();
		}
	}

	private static class UsesGetter implements DefsOrUsesGetter{

		@Override
		public Collection<Object> getByNode(DefUseCFG g, ASTNode n) {
			return g.getSymbolsUsedBy(n);
		}

		@Override
		public List<ASTNode> getBySymbol(UseDefGraph udg, String symbol) {
			List<ASTNode> nodes = new ArrayList<>();
			List<UseOrDefRecord> l = udg.getUsesAndDefsForSymbol(symbol);
			if(l != null){
				for(UseOrDefRecord r : l){
					if(!r.isDef())
						nodes.add(r.getAstNode());

				}
				return nodes;
			}
			else
				return null;
		}

		@Override
		public boolean test(UseOrDefRecord r) {
			return !r.isDef();
		}
	}

	private enum InsertedOrDeleted {
		INSERTED,
		DELETED;
	}

	/* ----------------------------------------------------
	 * 			EXTRACTION USING UDG INFORMATION
	   ---------------------------------------------------- */
	
	/*
	 * TODO Cache this, or make the whole thing not static
	 */
	public static Map<ASTNode, List<Object>> defsOrUsesByAstNodeMap(UseDefGraph udg, DefsOrUsesGetter getter){
		Map<ASTNode, List<Object>> returnValue = new HashMap<ASTNode, List<Object>>();
		for(String useOrDefSymbol : udg.getUseDefDict().keySet()){
			String symbol = useOrDefSymbol;
			for (UseOrDefRecord useOrDef : udg.getUsesAndDefsForSymbol(symbol)){
				ASTNode n = useOrDef.getAstNode();
				if(!returnValue.containsKey(n)){
					returnValue.put(n, new ArrayList<>());
				}

				if(getter.test(useOrDef)){
					returnValue.get(n).add(symbol);
				}
			}
		}
		return returnValue;
	}

	public static Map<ASTNode, List<Object>> defsByAstNodeMap(UseDefGraph udg){
		return defsOrUsesByAstNodeMap(udg, new DefsGetter());
	}

	public static Map<ASTNode, List<Object>> usesByAstNodeMap(UseDefGraph udg){
		return defsOrUsesByAstNodeMap(udg, new UsesGetter());
	}
	
	private static Map<ASTNode, List<Object>> defsOrUses(List<Statement> l, UseDefGraph udg, DefsOrUsesGetter getter){
		//		if(logger.isDebugEnabled()) logger.debug("Extracting writes/reads for " + l);
		Map<ASTNode, List<Object>> map = new HashMap<>();

		if(l != null)
			for(Statement s : l){
				//			if(logger.isDebugEnabled()) logger.debug("Input stmt: " + s);
				map.putAll(defsOrUses(s, udg , getter));
			}
		//		if(logger.isDebugEnabled()) logger.debug("Final return value:");
		//		if(logger.isDebugEnabled()) logger.debug(map);
		return map;
	}


	private static Map<ASTNode, List<Object>> defsOrUses(ASTNode n, UseDefGraph udg, DefsOrUsesGetter getter){
		Map<ASTNode, List<Object>> definesOrUses = new HashMap<>();
		defsOrUses(n, udg, getter, definesOrUses);
		//		if(logger.isDebugEnabled()) logger.debug("Inner returnValue " + definesOrUses);
		return definesOrUses;
	}

	
	/*
	 * Method used to extract defs/uses for a given ASTNode, 
	 * using the information present in the UseDefGraph
	 */
	private static void defsOrUses(ASTNode n, UseDefGraph udg, DefsOrUsesGetter getter, Map<ASTNode, List<Object>> map){
		/*
		 * TODO cache this stuff??
		 */
		List<Object> l = defsOrUsesByAstNodeMap(udg, getter).get(n);
		map.put(n, (l != null ? l : new ArrayList<Object>()));

		for (int i = 0 ; i < n.getChildCount() ; i++){
			defsOrUses(n.getChild(i), udg, getter, map);
		}
		
		//		DEBUGGING STUFF
		//		if(n instanceof Identifier){
		//		if(logger.isDebugEnabled()) logger.debug("BySymbol: " + n.getEscapedCodeStr());
		//		List<ASTNode> l = getter.getBySymbol(udg, n.getEscapedCodeStr());
		//		if(l != null){
		//			for (ASTNode node : l){
		//				if(logger.isDebugEnabled()) logger.debug(node + " " + node.getEscapedCodeStr());
		//				if(node.equals(n)){
		//					if(logger.isDebugEnabled()) logger.debug("THIS");
		//				}
		//			}
		//		}
		//	}

	}

	private static Map<ASTNode, List<Object>> insertedOrDeletedDefsOrUsesFineGrained(ASTDelta d, FunctionDiff f, DefsOrUsesGetter getter, InsertedOrDeleted insertedOrDeleted){
		/*
		 * TODO:
		 * - Solve problem with granularity of Joern defs and uses vs granularity of Gumtree
		 * - Updates handling doesn't work, check:
		 * 		/examples/android_security_bulletin/07-2016/CVE-2015-8892/new/boot_verifier.c
		 * 		where SPEW becomes CRITICAL
		 */

		if(!f.getAllDeltas().contains(d))
			throw new RuntimeException("The ASTDelta d must be a delta contained in the function f");

		String debug1 = (getter instanceof DefsGetter ? "write" : "read");
		String debug2 = (insertedOrDeleted == InsertedOrDeleted.INSERTED ? "inserted" : "deleted");

		if(logger.isDebugEnabled()) logger.debug("");
		if(logger.isDebugEnabled()) logger.debug("Extracting " + debug2 + " " + debug1 + "s from " + d);
		//If we want the added ones we check the new statements, otherwise the old. Same for the DefUseCfg.
		Map<ASTNode, List<Object>> defsOrUses = defsOrUses(
				(insertedOrDeleted == InsertedOrDeleted.INSERTED ? d.getNewStatements() : d.getOriginalStatements()),
				(insertedOrDeleted == InsertedOrDeleted.INSERTED ? f.getNewGraphs().getUdg() : f.getOriginalGraphs().getUdg()),
				getter);

		Map<ASTNode, List<Object>> output = new HashMap<>();

		/*
		 * We study the fine-grained diff to understand which reads/writes 
		 * are moved/updated/deleted/inserted.
		 */
		for (ASTNode n : defsOrUses.keySet()){
			
			//ITree t = d.getFineGrainedDiff().getMapper().getMappedITree(n);
			if(logger.isDebugEnabled()) logger.debug("Node: " + n + " " + n.getEscapedCodeStr());
			if(logger.isDebugEnabled()) logger.debug("Inserted: " + d.getFineGrainedDiff().isInserted(n) + 
					" Moved: " + d.getFineGrainedDiff().isMoved(n) +
					" Deleted: " + d.getFineGrainedDiff().isDeleted(n) +
					" Updated: " + d.getFineGrainedDiff().isUpdated(n));
			if(logger.isDebugEnabled()) logger.debug(debug1 + "s: " + defsOrUses.get(n));

			//If the node is inserted and not moved (so it was not already present in the old file)
			if(insertedOrDeleted == InsertedOrDeleted.INSERTED){
				if (d.getFineGrainedDiff().isInserted(n)
						//&& !d.getFineGrainedDiff().isMoved(n)
						&& defsOrUses.get(n).size() != 0){
					output.put(n, defsOrUses.get(n));
				}
				
				if(d.getFineGrainedDiff().isUpdated(n)){
					//We are extracting inserted uses/defs, so t is an ITree from the new file
					//We extract the corresponding old ITree and then ASTNode
					
					//ITree oldItree = d.getFineGrainedDiff().getMappings().getSrc(t);
					//ASTNode oldNode = d.getFineGrainedDiff().getMapper().getMappedASTNode(oldItree);
					ASTNode oldNode = d.getFineGrainedDiff().getMappedNode(n);
					
					List<Object> newDefsOrUses = defsOrUses.get(n);
					Map<ASTNode, List<Object>> oldDefsOrUsesMap = defsOrUses(d.getOriginalStatements(), f.getOriginalGraphs().getUdg(), getter);
					List<Object> oldDefsOrUses = oldDefsOrUsesMap.get(oldNode);
					if(logger.isDebugEnabled()) logger.debug("Old node: " + oldNode + " " + oldNode.getEscapedCodeStr());
					if(logger.isDebugEnabled()) logger.debug("Old " + debug1 + "s: " + oldDefsOrUses);
					
					//Removing the old ones from the new ones to keep only the newly inserted ones
					if(newDefsOrUses.size() != 0 && oldDefsOrUses.size() != 0){
						newDefsOrUses.removeAll(oldDefsOrUses);
						if(newDefsOrUses.size() != 0){
							output.put(n, newDefsOrUses);
						}
					} else if (newDefsOrUses.size() != 0){
						output.put(n, newDefsOrUses);
					}
					if(logger.isDebugEnabled()) logger.debug("Difference: " + newDefsOrUses);
				}
			}
			
			if(insertedOrDeleted == InsertedOrDeleted.DELETED){
				if (d.getFineGrainedDiff().isDeleted(n)
						//&& !d.getFineGrainedDiff().isMoved(n)
						&& defsOrUses.get(n).size() != 0){
					output.put(n, defsOrUses.get(n));
				}
				
				if(d.getFineGrainedDiff().isUpdated(n)){
					//We are extracting deleted uses/defs, so t is an ITree from the original file
					//We extract the corresponding new ITree and then ASTNode
					
					//ITree newItree = d.getFineGrainedDiff().getMappings().getDst(t);
					//ASTNode newNode = d.getFineGrainedDiff().getMapper().getMappedASTNode(newItree);
					ASTNode newNode = d.getFineGrainedDiff().getMappedNode(n);
					
					Map<ASTNode, List<Object>> newDefsOrUsesMap = defsOrUses(d.getNewStatements(), f.getNewGraphs().getUdg(), getter);
					List<Object> newDefsOrUses = newDefsOrUsesMap.get(newNode);
					List<Object> oldDefsOrUses = defsOrUses.get(n);
					if(logger.isDebugEnabled()) logger.debug("New node: " + newNode + " " + newNode.getEscapedCodeStr());
					if(logger.isDebugEnabled()) logger.debug("New " + debug1 + "s: " + newDefsOrUses);
					
					//Removing the new ones from the old ones to keep only the deleted ones
					if(newDefsOrUses.size() != 0 && oldDefsOrUses.size() != 0){
						oldDefsOrUses.removeAll(newDefsOrUses);
						if(oldDefsOrUses.size() != 0){
							output.put(n, oldDefsOrUses);
						}
					} else if (oldDefsOrUses.size() != 0){
						output.put(n, oldDefsOrUses);
					}
					if(logger.isDebugEnabled()) logger.debug("Difference: " + oldDefsOrUses);
				}
			}

			/*
			 * TODO if it's updated? You can still remove/add stuff.
			 * - In this case I have to manually do the difference between old and new reads/writes.
			 */

		}
		return output;

		//OLD VERSION DIVIDED BY DELTA TYPE
		//		if(d.getType() == TYPE.CHANGE){
		//ALL THE STUFF ABOVE
		//		} else if (d.getType() == TYPE.INSERT){
		//			/*
		//			 * If the delta is an insert then there is no fine-grained diff,
		//			 * all the reads/writes are inserted.
		//			 */
		//			for (ASTNode n : defsOrUses.keySet()){
		//				if (defsOrUses.get(n).size() != 0){
		//					output.put(n, defsOrUses.get(n));
		//				}
		//			}
		//			return output;
		//		} else if (d.getType() == TYPE.DELETE){
		//			/*
		//			 * If the delta is a delete then there is no fine-grained diff,
		//			 * all the reads/writes are deleted.
		//			 */
		//			for (ASTNode n : defsOrUses.keySet()){
		//				if (defsOrUses.get(n).size() != 0){
		//					output.put(n, defsOrUses.get(n));
		//				}
		//			}
		//			return output;
		//		} else 
		//			throw new RuntimeException("This delta type is unknown.");
	}

	
	
	/* --------------------------------------------------------
	 * DEFS OR USES EXTRACTION USING DEF-USE-CFG INSTEAD OF UDG
	   -------------------------------------------------------- */
	
	private static Map<ASTNode, List<Object>> defsOrUses(List<Statement> l, DefUseCFG ducfg, DefsOrUsesGetter getter){
		//		if(logger.isDebugEnabled()) logger.debug("Extracting writes/reads for " + l);
		Map<ASTNode, List<Object>> map = new HashMap<>();

		if(l != null)
			for(Statement s : l){
				//			if(logger.isDebugEnabled()) logger.debug("Input stmt: " + s);
				map.putAll(defsOrUses(s, ducfg , getter));
			}
		//		if(logger.isDebugEnabled()) logger.debug("Final return value:");
		//		if(logger.isDebugEnabled()) logger.debug(map);
		return map;
	}


	private static Map<ASTNode, List<Object>> defsOrUses(ASTNode n, DefUseCFG ducfg, DefsOrUsesGetter getter){
		Map<ASTNode, List<Object>> definesOrUses = new HashMap<>();
		defsOrUses(n, ducfg, getter, definesOrUses);
		//		if(logger.isDebugEnabled()) logger.debug("Inner returnValue " + definesOrUses);
		return definesOrUses;
	}

	
	/*
	 * Method used to extract defs/uses for a given Statement, using
	 * DefUseCfg information.
	 */
	private static void defsOrUses(ASTNode n, DefUseCFG ducfg, DefsOrUsesGetter getter, Map<ASTNode, List<Object>> map){
		// if(logger.isDebugEnabled()) logger.debug("Current: " + n + " " + n.getEscapedCodeStr());
		// if(logger.isDebugEnabled()) logger.debug("Defs/Uses: " + getter.getByNode(ducfg, n));

		//		THIS WORKS ONLY FOR STATEMENTS AND CONDITIONS
		map.put(n, (List<Object>) getter.getByNode(ducfg, n));
				
				for (int i = 0 ; i < n.getChildCount() ; i++){
					//if(s.getChild(i) instanceof Statement)
						defsOrUses(n.getChild(i), ducfg, getter, map);
				}
				
	}
	
	/*
	 * The difference between this method and the fine-grained one is that this one will not consider the information of the
	 * fine grained diff: it will just make the difference between old and new affected statements Use/Def information. 
	 */
	public static List<String> insertedOrDeletedDefsOrUsesCoarseGrained(ASTDelta d, FunctionDiff f, DefsOrUsesGetter getter, InsertedOrDeleted insertedOrDeleted){

		if(!f.getAllDeltas().contains(d))
			throw new RuntimeException("The ASTDelta d must be a delta contained in the function f");

		String debug1 = (getter instanceof DefsGetter ? "write" : "read");
		String debug2 = (insertedOrDeleted == InsertedOrDeleted.INSERTED ? "inserted" : "deleted");

		if(logger.isDebugEnabled()) logger.debug("");
		if(logger.isDebugEnabled()) logger.debug("Extracting " + debug2 + " " + debug1 + "s from " + d);
		//If we want the added ones we check the new statements, otherwise the old. Same for the DefUseCfg.
		Map<ASTNode, List<Object>> originalDefsOrUses = defsOrUses(
				d.getOriginalStatements(),
				f.getOriginalGraphs().getDefUseCfg(),
				getter);
		
		
		Map<ASTNode, List<Object>> newDefsOrUses = defsOrUses(
				d.getNewStatements(),
				f.getNewGraphs().getDefUseCfg(),
				getter);
		
		Set<String> originalDefsOrUsesSymbols = new HashSet<>(); 
		for (List<Object> l : originalDefsOrUses.values())
			for(Object s : l)
				originalDefsOrUsesSymbols.add((String) s);
		
		
		Set<String> newDefsOrUsesSymbols = new HashSet<>(); 
		for (List<Object> l : newDefsOrUses.values())
			for(Object s : l)
				newDefsOrUsesSymbols.add((String) s);
		
		//if(logger.isDebugEnabled()) logger.debug("Original " + debug1 + "s:" + originalDefsOrUsesSymbols);
		//if(logger.isDebugEnabled()) logger.debug("New " + debug1 + "s:" + newDefsOrUsesSymbols);
		
		if(logger.isDebugEnabled()) logger.debug((insertedOrDeleted == InsertedOrDeleted.INSERTED ? 
				com.google.common.collect.Sets.difference(newDefsOrUsesSymbols, originalDefsOrUsesSymbols) :
					com.google.common.collect.Sets.difference(originalDefsOrUsesSymbols, newDefsOrUsesSymbols)));
		
		List<String> output = null;
		return output;
	}
	
}



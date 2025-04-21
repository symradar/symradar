package tools.safepatch.util;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ast.ASTNode;
import ast.expressions.Identifier;
import ast.functionDef.FunctionDef;
import ast.functionDef.ReturnType;
import ast.statements.BlockStarter;
import ast.statements.CompoundStatement;
import ast.statements.DoStatement;
import ast.statements.ElseStatement;
import ast.statements.ExpressionStatement;
import ast.statements.ForStatement;
import ast.statements.IdentifierDeclStatement;
import ast.statements.IfStatement;
import ast.statements.Statement;
import ast.statements.SwitchStatement;
import ast.statements.WhileStatement;
import cdg.CDG;
import cdg.CDGEdge;
import cdg.DominatorTree;
import cfg.CFG;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;
import ddg.DataDependenceGraph.DDG;
import ddg.DataDependenceGraph.DDGDifference;
import ddg.DataDependenceGraph.DefUseRelation;
import ddg.DefUseCFG.DefUseCFG;
import difflib.Chunk;
import tools.safepatch.rw.ReadWriteTable;
import udg.symbols.UseOrDefSymbol;
import udg.useDefGraph.UseDefGraph;
import udg.useDefGraph.UseOrDefRecord;

/**
 * @author Eric Camellini
 * 
 */
public class JoernUtil {

	private static final Logger logger = LogManager.getLogger();
	
	public static String genCode(ASTNode node){
		String code = genCodeHelper(node, 0);
		if(logger.isDebugEnabled()) logger.debug("\n" + code);
		return code;
	}
	
	
	private static String genCodeHelper(ASTNode node, int indentCount){
		
		if(node instanceof FunctionDef){
			FunctionDef f = (FunctionDef) node;
			return f.getChild(1).getEscapedCodeStr() + " " + f.getEscapedCodeStr() + "\n" + genCodeHelper(f.getContent(), indentCount++);		
		}
		
		if(node instanceof CompoundStatement){
						
			List<ASTNode> childs = ((CompoundStatement) node).getStatements();
			String c = Util.repeatString(" ", indentCount*2) + "{\n";
			indentCount++;
			for (ASTNode child : childs)
				c += genCodeHelper(child, indentCount) + "\n";
			indentCount--;
			return c + Util.repeatString(" ", indentCount*2) + "}";
			
		}

		

		if(node instanceof BlockStarter){
			
			if(node instanceof IfStatement){
				IfStatement ifStmt = (IfStatement) node;

					ASTNode body = ifStmt.getChild(1);
					String ifCode = Util.repeatString(" ", indentCount*2) + ifStmt.getEscapedCodeStr() + "\n" + genCodeHelper(body, 
							((body instanceof CompoundStatement) ? indentCount : ++indentCount)); 
		
				if (!(body instanceof CompoundStatement))
					indentCount--;
				
				//Handle else block
				String elseCode = "";
				if (ifStmt.getElseNode() != null)
					elseCode = genCodeHelper(ifStmt.getElseNode(), indentCount);
				
				return ifCode + elseCode;
			}

			if(node instanceof ElseStatement){
				ElseStatement elseStmt = (ElseStatement) node;
				ASTNode body = elseStmt.getChild(0);
				return "\n" + Util.repeatString(" ", indentCount*2) + elseStmt.getEscapedCodeStr() + "\n" + genCodeHelper(body, 
						((body instanceof CompoundStatement) ? indentCount : ++indentCount)) + "\n";

			}

			if(node instanceof DoStatement){
				DoStatement doStmt = (DoStatement) node;
				ASTNode body = doStmt.getChild(0);
				return Util.repeatString(" ", indentCount*2) + doStmt.getEscapedCodeStr() + "\n" + genCodeHelper(body, 
						((body instanceof CompoundStatement) ? indentCount : ++indentCount)) 
				+ " while ( " + doStmt.getChild(1).getEscapedCodeStr() + " ) ;\n";
			}

			if(node instanceof ForStatement){
				ForStatement forStmt = (ForStatement) node;
				
				ASTNode body = forStmt.getChild(forStmt.getChildCount() - 1);
				return Util.repeatString(" ", indentCount*2) + forStmt.getEscapedCodeStr() + "\n" + genCodeHelper(body, 
						((body instanceof CompoundStatement) ? indentCount : ++indentCount)) + "\n";
			}

			if(node instanceof WhileStatement){
				WhileStatement whileStmt = (WhileStatement) node;
				ASTNode body = whileStmt.getChild(1);
				return Util.repeatString(" ", indentCount*2) + whileStmt.getEscapedCodeStr() + "\n" + genCodeHelper(body, 
						((body instanceof CompoundStatement) ? indentCount : ++indentCount)) + "\n";

			}

			if(node instanceof SwitchStatement){
				SwitchStatement switchStmt = (SwitchStatement) node;
				ASTNode body = switchStmt.getChild(1);
				return Util.repeatString(" ", indentCount*2) + switchStmt.getEscapedCodeStr() + "\n" + genCodeHelper(body, 
						((body instanceof CompoundStatement) ? indentCount : ++indentCount)) + "\n";

			}

			//TODO: (?) Add try and catch to support C++
		}

		return Util.repeatString(" ", indentCount*2) + node.getEscapedCodeStr()
			+ (!(node instanceof ExpressionStatement) ? "" : " ;");
	}
	
	public static List<ASTNode> getLeaves(ASTNode node){
		List<ASTNode> leavesList = new ArrayList<ASTNode>();
		getLeaves(node, leavesList);
		return leavesList;
	}
	
	public static void getLeaves(ASTNode node, List<ASTNode> leavesList){
		if(node.getChildCount() == 0)
			leavesList.add(node);
		
		for(int i = 0 ; i < node.getChildCount(); i++)
			getLeaves(node.getChild(i), leavesList);
	}

	public static void printASTNodeAndChildren(ASTNode node){
		printASTNodeAndChildren(node, 0);
	}
	
	public static void printASTNodeAndChildren(ASTNode node, int indent){
		printASTNode(node, indent);
		indent++;
		for(int i = 0 ; i < node.getChildCount(); i++)
			printASTNodeAndChildren(node.getChild(i), indent);
	}


	public static void printASTNode(ASTNode node, int indent){
		System.out.println(Util.repeatString(" ", indent*2) + 
				node.toString() + 
				//" - Code: " + node.getEscapedCodeStr() + 
				" - GumtreeId: " + node.getIDForGumTree() +
				" - Loc: " + node.getLocation() + 
				" - Leaf: " + node.isLeaf() + 
				" - InCFG: " + node.isInCFG());
	}
	
	public static void printASTNode(ASTNode node){
		JoernUtil.printASTNode(node, 0);
	}
	
	/*
	 * GRAPH DISPLAYING FUNCTIONS:
	 */
	public static void printCfgWithASTNodes(CFG cfg){
		for (CFGNode n : cfg.getVertices()){
			System.out.println(n.getProperties());
			if(n instanceof ASTNodeContainer){
				ASTNodeContainer nc = (ASTNodeContainer) n;
				printASTNode(nc.getASTNode(), 0);
			}

		}
	}
	
	public static void printUDG(UseDefGraph udg){
		Iterator<String> it = udg.getUseDefDict().getKeySetIterator();

		while(it.hasNext()){
			String symbol = it.next();
			System.out.println("Symbol: " + symbol);
			List<UseOrDefRecord> l = udg.getUseDefDict().get(symbol);
			for(UseOrDefRecord r : l){
				System.out.println("isDef: " + r.isDef());
				printASTNode(r.getAstNode(), 0);
			}
			System.out.println();
		}

	}
	
	public static void printDefUseCFG(DefUseCFG defUseCfg){
		for(Object s: defUseCfg.getStatements()){
			System.out.println("Def-use cfg node: " + s + " " + ((s instanceof ASTNode) ? ((ASTNode) s).getEscapedCodeStr() : ""));
			System.out.println("Symbols defined: " + defUseCfg.getSymbolsDefinedBy(s));
			System.out.println("Symbols used: " + defUseCfg.getSymbolsUsedBy(s));
			System.out.println("Child blocks: " + defUseCfg.getChildBlocks().get(s));
			System.out.println("Parent blocks: " + defUseCfg.getParentBlocks().get(s));
			System.out.println();
		}
	}
	
	public static void printCdg(CDG cdg){
		System.out.println(cdg);
		for (CDGEdge e: cdg.getEdges()){
			System.out.println(e + " " + e.getSource() + " " + e.getDestination());
		}
	}
	
	public static void printDominatorTree(DominatorTree<CFGNode> dom){
		for (CFGNode n : dom.getVertices()){
			System.out.println("CFG Node " + n);
			System.out.println("Dominator: " + dom.getDominator(n));
			System.out.println("Dominance frontier" + dom.dominanceFrontier(n));
			System.out.println();
		}
	}
	
	public static void printDefUseRelation(DefUseRelation r){
		System.out.println("Relation: " + r + "\nSymbol: " + r.symbol + "\nSource: " + r.src + "\nDest: " + r.dst);
		System.out.println();
	}
	
	public static void printDdg(DDG ddg){
		for (DefUseRelation r : ddg.getDefUseEdges()){
			JoernUtil.printDefUseRelation(r);
		}
	}
	
	public static void printDdgDiff(DDGDifference diff){
		System.out.println("\n>>RELS TO ADD:");
		for (DefUseRelation r : diff.getRelsToRemove()){
			JoernUtil.printDefUseRelation(r);
		}
		System.out.println("\n>>RELS TO REMOVE");
		for (DefUseRelation r : diff.getRelsToAdd()){
			JoernUtil.printDefUseRelation(r);
		}
	}
	
	public static void printReadWriteTable(ReadWriteTable table){
		System.out.println();
		System.out.println("READS:");
		for (ASTNode n : table.getReadTable().keySet()){
			printASTNode(n);
			System.out.println(table.getReadSymbolValues(n));
			System.out.println("> UseOrDefSymbols: ");
			for (UseOrDefSymbol s : table.getReadTable().get(n)){
				System.out.println(s);
				System.out.println(">> Identifiers: ");
				for (Identifier i : s.getIdentifiers())
					printASTNode(i);
				System.out.println(">> ASTNode where symbol was added: ");
				printASTNode(s.getNode());
			}
			System.out.println();
		}
		
		System.out.println();
		System.out.println("WRITES:");
		for (ASTNode n : table.getWriteTable().keySet()){
			printASTNode(n);
			System.out.println(table.getWriteSymbolValues(n));
			System.out.println("> UseOrDefSymbols: ");
			for (UseOrDefSymbol s : table.getWriteTable().get(n)){
				System.out.println(s);
				System.out.println(">> Identifier: ");
				for (Identifier i : s.getIdentifiers())
					printASTNode(i);
				System.out.println(">> ASTNode where symbol was added: ");
				printASTNode(s.getNode());
			}
			System.out.println();
		}
			
	}
	
/*
 * USELESS STUFF
 * 
 */
	
//	public static ASTNode getBlockStarterBody(BlockStarter blockStarter){
//		if(blockStarter instanceof IfStatement){
//			IfStatement ifStmt = (IfStatement) blockStarter;
//			return ifStmt.getChild(1);
//		}
//
//		if(blockStarter instanceof ElseStatement){
//			ElseStatement elseStmt = (ElseStatement) blockStarter;
//			return elseStmt.getChild(0);
//		}
//
//		if(blockStarter instanceof DoStatement){
//			DoStatement doStmt = (DoStatement) blockStarter;
//			return doStmt.getChild(0);
//		}
//
//		if(blockStarter instanceof ForStatement){
//			ForStatement forStmt = (ForStatement) blockStarter;
//			return forStmt.getChild(3);
//		}
//
//		if(blockStarter instanceof WhileStatement){
//			WhileStatement whileStmt = (WhileStatement) blockStarter;
//			return whileStmt.getChild(1);
//		}
//
//		if(blockStarter instanceof SwitchStatement){
//			SwitchStatement switchStmt = (SwitchStatement) blockStarter;
//			return switchStmt.getChild(1);
//		}
//		
//		/*
//		 * TODO: Try catch
//		 */
//		
//		return null;
//	}
	
//	public static List<ASTNode> getChildStatements(ASTNode node){
//		List<ASTNode> childStatements = null;
//		if (node instanceof BlockStarter){
//			
//			ASTNode body = getBlockStarterBody((BlockStarter) node);
//			if (body instanceof CompoundStatement)
//				childStatements = ((CompoundStatement) body).getStatements();
//			else{
//				childStatements = new ArrayList<ASTNode>();
//				childStatements.add(body);
//			}
//		}
//		
//		if (node instanceof CompoundStatement )
//			childStatements = ((CompoundStatement) node).getStatements();
//		
//		return childStatements;
//	}
	
//	public static List<ASTNode> findCandidates(ASTNode subTreeStartNode, ASTNode targetTreeStartNode, List<ASTNode> candidates){
//		/*
//		 * TODO this maybe is useless
//		 */
//		System.out.println(targetTreeStartNode + " " + targetTreeStartNode.getEscapedCodeStr());
//		if(subTreeStartNode.getEscapedCodeStr().equals(targetTreeStartNode.getEscapedCodeStr())){
//			candidates.add(targetTreeStartNode);
//		}
//		for(int i = 0; i < targetTreeStartNode.getChildCount(); i++)
//			findCandidates(subTreeStartNode, targetTreeStartNode.getChild(i), candidates);
//		
//		return candidates;
//	}
	
	
}

package tools.safepatch.rw;

import java.util.Collection;
import java.util.HashSet;
import java.util.List;

import ast.ASTNode;
import ast.expressions.Argument;
import ast.expressions.Expression;
import ast.functionDef.FunctionDef;
import ast.statements.BlockStarter;
import ast.statements.Condition;
import ast.statements.Statement;
import cfg.CFG;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;
import udg.ASTNodeASTProvider;
import udg.symbols.UseOrDefSymbol;
import udg.useDefAnalysis.ASTDefUseAnalyzer;
import udg.useDefGraph.UseDefGraph;
import udg.useDefGraph.UseOrDef;


/**
 * @author Eric Camellini
 *
 */
public class ASTToRWTableConverter {

	ASTDefUseAnalyzer astAnalyzer = new ASTDefUseAnalyzer();

	public ReadWriteTable convert(ASTNode startNode)
	{
		// Incrementally create a ReadWriteTable by generating
		// UseOrDefs for each statement ASTNode separately and adding those
		// to the ReadWriteTable

		ReadWriteTable readWriteTable = new ReadWriteTable();
		handleNode(startNode, readWriteTable);
		return readWriteTable;
	}

	private void handleNode(ASTNode node, ReadWriteTable readWriteTable){
		/*
		 * TODO 
		 * - Figure out for which kind of nodes it makes sense to do it instead
		 * of doing it on all nodes
		 */
//		if(node.isInCFG() ||
//				node instanceof Condition ||
//				node instanceof Statement ||
		//				node instanceof FunctionDef){
		ASTNodeASTProvider provider = new ASTNodeASTProvider();
		provider.setNode(node);
		Collection<UseOrDef> usesAndDefs = astAnalyzer.analyzeAST(provider);
		addToReadWriteTable(readWriteTable, usesAndDefs, node);
		traverseChildren(node, readWriteTable);
		//		} else {
		//			/*
		//			 * If the node is not of the kind we are interested in let's still check its children
//			 */
//			traverseChildren(node, readWriteTable);
//		}
	}

	private void traverseChildren(ASTNode node, ReadWriteTable readWriteTable) {
		for (int i = 0 ; i < node.getChildCount() ; i++)
			handleNode(node.getChild(i), readWriteTable);
	}

	private void addToReadWriteTable(ReadWriteTable readWriteTable, Collection<UseOrDef> usesAndDefs, ASTNode node) {

		HashSet<UseOrDefSymbol> insertedForASTNodeDef = new HashSet<UseOrDefSymbol>();
		HashSet<UseOrDefSymbol> insertedForASTNodeUse = new HashSet<UseOrDefSymbol>();

		for(UseOrDef useOrDef : usesAndDefs){
			ASTNodeASTProvider astProvider = (ASTNodeASTProvider) useOrDef.astProvider;
			ASTNode useOrDefNode = astProvider.getASTNode();
			UseOrDefSymbol useOrDefSymbol = useOrDef.symbol;

			if (useOrDef.isDef)
			{

				if (!insertedForASTNodeDef.contains(useOrDefSymbol))
				{
					readWriteTable.addWrite(node, useOrDefSymbol); 
					insertedForASTNodeDef.add(useOrDefSymbol);
				}

				if (useOrDefNode != null && useOrDefNode != node)
					readWriteTable.addWrite(useOrDefNode, useOrDefSymbol);
			}
			else
			{

				if (!insertedForASTNodeUse.contains(useOrDefSymbol))
				{
					readWriteTable.addRead(node, useOrDefSymbol);
					insertedForASTNodeUse.add(useOrDefSymbol);
				}

				if (useOrDef.astProvider != null
						&& useOrDefNode != node)
					readWriteTable.addRead(useOrDefNode, useOrDefSymbol);
			}
		}

	}

}

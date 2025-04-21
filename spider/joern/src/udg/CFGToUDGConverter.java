package udg;

import java.util.Collection;
import java.util.HashSet;
import java.util.List;

import udg.symbols.UseOrDefSymbol;
import udg.useDefAnalysis.ASTDefUseAnalyzer;
import udg.useDefGraph.UseDefGraph;
import udg.useDefGraph.UseOrDef;
import ast.ASTNode;
import cfg.CFG;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;

public class CFGToUDGConverter
{

	ASTDefUseAnalyzer astAnalyzer = new ASTDefUseAnalyzer();

	public void addTaintSource(String callee, int argNum)
	{
		astAnalyzer.addTaintSource(callee, argNum);
	}

	public UseDefGraph convert(CFG cfg)
	{
		// Incrementally create a UseDefGraph by generating
		// UseOrDefs for each statement separately and adding those
		// to the UseDefGraph

		UseDefGraph useDefGraph = new UseDefGraph();

		List<CFGNode> statements = cfg.getVertices();

		for (CFGNode cfgNode : statements)
		{
			// skip empty blocks
			if (cfgNode instanceof ASTNodeContainer)
			{
				ASTNode statementNode = ((ASTNodeContainer) cfgNode)
						.getASTNode();
				ASTNodeASTProvider provider = new ASTNodeASTProvider();
				provider.setNode(statementNode);
				Collection<UseOrDef> usesAndDefs = astAnalyzer
						.analyzeAST(provider);
				addToUseDefGraph(useDefGraph, usesAndDefs, statementNode);
			}
		}

		return useDefGraph;
	}

	private void addToUseDefGraph(UseDefGraph useDefGraph,
			Collection<UseOrDef> usesAndDefs, ASTNode statementNode)
	{
		HashSet<String> insertedForStatementDef = new HashSet<String>();
		HashSet<String> insertedForStatementUse = new HashSet<String>();

		for (UseOrDef useOrDef : usesAndDefs)
		{

			ASTNodeASTProvider astProvider = (ASTNodeASTProvider) useOrDef.astProvider;
			// CHECK?
			ASTNode useOrDefNode = astProvider.getASTNode();
			String useOrDefSymbolValue = useOrDef.symbol.getSymbolValue();
			
			if (useOrDef.isDef)
			{

				if (!insertedForStatementDef.contains(useOrDefSymbolValue))
				{
					useDefGraph.addDefinition(useOrDefSymbolValue, statementNode);
					insertedForStatementDef.add(useOrDefSymbolValue);
				}

				if (useOrDefNode != null && useOrDefNode != statementNode)
					useDefGraph.addDefinition(useOrDefSymbolValue, useOrDefNode);
			}
			else
			{

				if (!insertedForStatementUse.contains(useOrDefSymbolValue))
				{
					useDefGraph.addUse(useOrDefSymbolValue, statementNode);
					insertedForStatementUse.add(useOrDefSymbolValue);
				}

				// Add use-links from AST nodes to symbols
				if (useOrDef.astProvider != null
						&& useOrDefNode != statementNode)
					useDefGraph.addUse(useOrDefSymbolValue, useOrDefNode);
			}
		}
	}

}

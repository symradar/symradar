package tools.safepatch.util;

import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ast.ASTNode;
import ast.functionDef.FunctionDef;
import ast.statements.Condition;
import cfg.ASTToCFGConverter;
import cfg.CFG;
import ddg.CFGAndUDGToDefUseCFG;
import ddg.DDGCreator;
import ddg.DataDependenceGraph.DDG;
import ddg.DefUseCFG.DefUseCFG;
import tools.safepatch.tests.TestStringFunctionParser;
import udg.CFGToUDGConverter;
import udg.useDefGraph.UseDefGraph;

public class DDGUtil {

	private static final Logger logger = LogManager.getLogger();
	
	/**
	 * Takes the symbols used by the node and for every symbol
	 * goes backward in the data dependencies graph and finds all
	 * the definitions (statements) that reach it and puts them
	 * in the return list. This process is doing recursively.
	 * Example:
	 * 1. a = 1;
	 * 2. b = 2;
	 * 3. c = a + b;
	 * 4. d = 3;
	 * 5. if(c > 1){
	 * 		...
	 * 
	 * If we call this function on the condition at line 5
	 * it will return a list containing statements 3,2,1.
	 */
	public static Set<ASTNode> getAffectingDefs(ASTNode node, DDG ddg, DefUseCFG ducfg, boolean ignoreCConstants){
		if(logger.isDebugEnabled()) logger.debug("Starting DDG backward traversal for {}", node);
		Set<ASTNode> retval = new HashSet<ASTNode>();
		
		// This method will fill the retval list
		getAffectingDefsHelper(node, ddg, ducfg, ignoreCConstants, retval, new HashSet<ASTNode>());
		
		if(logger.isDebugEnabled()) logger.debug("Done. returning {}", retval);
		return retval;
	}
	
	private static void getAffectingDefsHelper(ASTNode node, DDG ddg, DefUseCFG ducfg, boolean ignoreCConstants, 
			Set<ASTNode> retval, HashSet<ASTNode> analyzedNodes){
		
		if(analyzedNodes.contains(node)){
			if(logger.isDebugEnabled()) logger.debug("skipping already analyzed node: {}", node);
			return;
		}
		else analyzedNodes.add(node);
		
		if(logger.isDebugEnabled()) logger.debug("handling node: {}", node);
		
		// We first look for the variable used by the statement
		Collection<Object> uses = ducfg.getSymbolsUsedBy(node);
		if(logger.isDebugEnabled()) logger.debug("Uses: {}", uses);

		// For every use
		for(Object use : uses){
			
			if(ignoreCConstants && ((String) use).toUpperCase().equals((String) use)) continue;
			
			String symbol = (String) use;
			if(logger.isDebugEnabled())  logger.debug("Considering use: {}", symbol);

			// we find the reaching defs and we add them to the list
			List<ASTNode> reachingDefs = ddg.getReachingDefs(node, symbol);
			if(logger.isDebugEnabled())  logger.debug("Reaching defs: {}", reachingDefs);
			retval.addAll(reachingDefs);

			// Then we recursively do the same for every reaching def
			for(ASTNode def : reachingDefs){
				getAffectingDefsHelper(def, ddg, ducfg, ignoreCConstants, retval, analyzedNodes);
			}
		}
	}
	
	/*
	 * TEST MAIN
	 */
	public static void main(String[] args) {
		
		String func = "void foo(int b, int c){\n" + 
				"  int a = 0;\n" +
				"  if(b > c){\n"
				+ "  a = 5;"
				+ "	 c = 0;\n"
				+ "} else {\n"
				+ "  if(b = 1) c = 5;\n"
				+ "}\n"
				+ "while(a != 0){\n" +
				"    if(a > b || c){\n" + 
				"      printf(\"Hello!\")\n" + 
				"    }\n"
				+ "	 a--;\n"
				+ "}\n" + 
				"}";

		System.out.println(func);
		TestStringFunctionParser p = new TestStringFunctionParser(func);
		FunctionDef f = p.parse();
		
		ASTToCFGConverter conv = new ASTToCFGConverter();
		CFG cfg = conv.convert(f);

		CFGToUDGConverter udgConv = new CFGToUDGConverter();
		UseDefGraph udg = udgConv.convert(cfg);

		CFGAndUDGToDefUseCFG useDefCfgCreator = new CFGAndUDGToDefUseCFG();
		DefUseCFG defUseCfg = useDefCfgCreator.convert(cfg, udg);

		DDGCreator ddgCreator = new DDGCreator();
		DDG ddg = ddgCreator.createForDefUseCFG(defUseCfg);
		
		System.out.println();
		Condition c = (Condition) f.getContent().getChild(f.getContent().getChildCount() - 1).getChild(1).getChild(0).getChild(0);
		System.out.println("Condition: ");
		System.out.println(c);
		
		System.out.println(getAffectingDefs(c, ddg, defUseCfg, true));
	}
}

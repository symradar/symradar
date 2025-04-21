package tools.safepatch.tests;

import static org.junit.Assert.*;

import org.junit.Test;

import ast.functionDef.FunctionDef;
import ast.functionDef.Parameter;
import ast.statements.ReturnStatement;
import cfg.ASTToCFGConverter;
import cfg.CFG;
import cfg.C.CCFG;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;
import tests.udg.CFGCreator;
import tools.safepatch.cfgimpl.CFGIterator;
import tools.safepatch.cfgimpl.ASTCFGMapping;
import tools.safepatch.diff.FunctionDiff;

public class CFGImplicationTests {

	String functionBase = 
			"void f(%s){\n"
			+ "%s"
			+ "}";
	
	private CFG extractCFG(String functionCode){
		TestStringFunctionParser p = new TestStringFunctionParser(functionCode);
		FunctionDef f = p.parse();
		ASTToCFGConverter c = new ASTToCFGConverter();
		return c.convert(f);
	}
	
	@Test
	public void mappingTest(){
		TestStringFunctionDiffParser p = new TestStringFunctionDiffParser(
				String.format(functionBase,"", "  return 0;\n"),
				String.format(functionBase,"", "  return -1;\n"));
		FunctionDiff f = p.parse();
		f.generateGraphs();
		CCFG originalCfg =  (CCFG) f.getOriginalGraphs().getCfg();
		CCFG newCfg =  (CCFG) f.getNewGraphs().getCfg();
		ASTCFGMapping m = new ASTCFGMapping(f);
		m.map();
		for(int i = 0 ; i < originalCfg.getVertices().size() ; i++){
			assertTrue(m.getMappedDstNode(originalCfg.getVertices().get(i)).equals(newCfg.getVertices().get(i)));
			assertTrue(m.getMappedSrcNode(newCfg.getVertices().get(i)).equals(originalCfg.getVertices().get(i)));
		}
		
	}
	
	@Test
	public void walkerTest(){
		String f = String.format(functionBase, "int a" , "  return 0;\n");
		CFG cfg = extractCFG(f);
		CFGIterator w = new CFGIterator(cfg);
		assertTrue(w.getCurrentNodeIndex() == 2);		
		assertTrue(((ASTNodeContainer) w.getCurrentNode()).getASTNode() instanceof Parameter);
		assertTrue(w.hasNext());
		assertTrue(((ASTNodeContainer) w.getNext()).getASTNode() instanceof ReturnStatement);
		w.moveToNext();
		assertTrue(w.getCurrentNodeIndex() == 3);
		assertTrue(((ASTNodeContainer) w.getCurrentNode()).getASTNode() instanceof ReturnStatement);
		assertFalse(w.hasNext());
		assertTrue(w.getNext() == null);
	}
	
}

package tools.safepatch.tests;

import static org.junit.Assert.*;

import org.junit.Test;

import ast.functionDef.FunctionDef;
import ast.statements.CompoundStatement;
import ast.statements.ReturnStatement;
import tools.safepatch.heuristics.CodeHeuristics;

public class CodeHeuristicsTest {
	
	final String function = 
			"int f(){\n"
			+ "%s\n"
			+ "}";
	
	
	CompoundStatement extractCompound(String compoundCode){
		String testFunction = String.format(function, compoundCode);
		TestStringFunctionParser p = new TestStringFunctionParser(testFunction);
		p.activateVisualization();
		FunctionDef f = p.parse();
		return f.getContent();
	}
	
	@Test
	public void testExtraction(){
		String compoundCode = "return -ASD;";
		CompoundStatement c = extractCompound(compoundCode);
		assertTrue(c.getStatements().size() == 1);
		assertTrue(c.getStatements().get(0) instanceof ReturnStatement);
	}
	
	@Test
	public void returnErrorTest(){
		CodeHeuristics h = new CodeHeuristics();
		h.setReturnErrorScore(1);
		
		String compoundCode = "return -1;";
		ReturnStatement r = (ReturnStatement) extractCompound(compoundCode).getChild(0);
		
		assertTrue(h.errorHeuristicsScore(r) == 1);
		assertTrue(h.isError(r));
		
		h.setReturnErrorScore(2);
		assertFalse(h.isError(r));
		
		compoundCode = "return error_variable;";
		r = (ReturnStatement) extractCompound(compoundCode).getChild(0);
		assertTrue(h.errorHeuristicsScore(r) == 1);
		assertFalse(h.isError(r));
		
		compoundCode = "return -errorvar;";
		r = (ReturnStatement) extractCompound(compoundCode).getChild(0);
		assertTrue(h.errorHeuristicsScore(r) == 2);
		assertTrue(h.isError(r));
		
		compoundCode = "return -EINVAL;";
		r = (ReturnStatement) extractCompound(compoundCode).getChild(0);
		assertTrue(h.errorHeuristicsScore(r) == 2);
		assertTrue(h.isError(r));
		
		compoundCode = "return EINVAL;";
		r = (ReturnStatement) extractCompound(compoundCode).getChild(0);
		assertTrue(h.errorHeuristicsScore(r) == 1);
		assertFalse(h.isError(r));
	}
	
	@Test
	public void compoundErrorTest(){
		CodeHeuristics h = new CodeHeuristics();
		
		String compoundCode =  "  pr_err(\"Invalid user input\\n\");\n" + 
							   "  return -EINVAL;\n";
		CompoundStatement c = extractCompound(compoundCode);
		assertTrue(h.errorHeuristicScore(c.getStatements()) == 4);
		assertTrue(h.isError(c.getStatements()));
		
		
		compoundCode =  "  dprintf(CRITICAL, \"Integer overflow detected in bootimage header fields\\n\");\n" + 
						"  return -1;";
		c = extractCompound(compoundCode);
		assertTrue(h.errorHeuristicScore(c.getStatements()) == 4);
		assertTrue(h.isError(c.getStatements()));
		

		compoundCode =  "ND_PRINTK(2, warn, \"RA: Got route advertisement with lower hop_limit than current\\n\");\n";
		c = extractCompound(compoundCode);
		assertTrue(h.errorHeuristicScore(c.getStatements()) == 2);
		assertTrue(h.isError(c.getStatements()));
		
		compoundCode =  "goto data_overrun_error;";
		c = extractCompound(compoundCode);
		assertTrue(h.errorHeuristicScore(c.getStatements()) == 2);
		assertTrue(h.isError(c.getStatements()));
	}
}

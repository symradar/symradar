package tools.safepatch.tests;

import static org.junit.Assert.*;

import java.io.IOException;

import org.junit.Before;
import org.junit.Test;

import ast.functionDef.FunctionDef;
import ast.statements.Condition;
import cfg.ASTToCFGConverter;
import tools.safepatch.conditions.ConditionToBooleanFormula;
import tools.safepatch.conditions.ConditionToBooleanFormula.CONVERSION_METHOD;
import tools.safepatch.conditions.ConditionDiff;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.fgdiff.ASTDiff.ACTION_TYPE;
import udg.CFGToUDGConverter;

/**
 * @author Eric Camellini
 *
 */
public class ConditionsTest { 
	
	@Before
	public void init()
	{
		
	}
	
	@Test
	public void testConditionDiffCreation() {
		String oldCondition = "a";
		String newCondition = "a < b";
		ConditionDiff d = extractConditionDiff(oldCondition, newCondition);
		assertTrue(d != null);
	}
	
	@Test
	public void testBooleanDiff(){
		String oldCondition = "a < b";
		String newCondition = "(" + oldCondition + ") && (a > c)";
		ConditionDiff d = extractConditionDiff(oldCondition, newCondition);
		assertTrue(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		
		oldCondition = newCondition;
		newCondition = "(" + oldCondition + ") && d";
		d = extractConditionDiff(oldCondition, newCondition);
		assertTrue(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		
		oldCondition = newCondition;
		newCondition =  "((e == f) || g) && (" + oldCondition + ")";
		d = extractConditionDiff(oldCondition, newCondition);
		assertTrue(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		
		oldCondition = newCondition;
		newCondition =  "h || (" + oldCondition + ")";
		d = extractConditionDiff(oldCondition, newCondition);
		assertFalse(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		assertTrue(d.getBooleanAndArithmeticDiff().originalImpliesNew());
		
		//False because of the operator change
		oldCondition = "(a < b) || c";
		newCondition = "((a > b) || c) && d";
		d = extractConditionDiff(oldCondition, newCondition);
		assertFalse(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		
		//False because of the identifier change
		oldCondition = "(a < b) || c";
		newCondition = "((g < b) || c) && d";
		d = extractConditionDiff(oldCondition, newCondition);
		assertFalse(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		
		oldCondition = "!n";
		newCondition = "!n && anotherBool";
		d = extractConditionDiff(oldCondition, newCondition);
		assertTrue(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		
		oldCondition = "A";
		newCondition = "A && B";
		d = extractConditionDiff(oldCondition, newCondition);
		assertTrue(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		
		oldCondition = "A";
		newCondition = "A && B && C";
		d = extractConditionDiff(oldCondition, newCondition);
		assertTrue(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		
		oldCondition = "A";
		newCondition = "B && A";
		d = extractConditionDiff(oldCondition, newCondition);
		assertTrue(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		
		oldCondition = "A";
		newCondition = "C && B && A";
		d = extractConditionDiff(oldCondition, newCondition);
		assertTrue(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		
		oldCondition = "A";
		newCondition = "A || B";
		d = extractConditionDiff(oldCondition, newCondition);
		assertTrue(d.getBooleanAndArithmeticDiff().originalImpliesNew());
		assertFalse(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		
		oldCondition = "(a < b) || c";
		newCondition = "(a < b) || c && d";
		d = extractConditionDiff(oldCondition, newCondition);
		assertTrue(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		
		oldCondition = "(a < b) || c";
		newCondition = "(a < b) || c";
		d = extractConditionDiff(oldCondition, newCondition);
		assertTrue(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		assertTrue(d.getBooleanAndArithmeticDiff().originalImpliesNew());
		assertTrue(d.getBooleanAndArithmeticDiff().biImplication());
		
		oldCondition = "(a < b)";
		newCondition = "!(a < b)";
		d = extractConditionDiff(oldCondition, newCondition);
		assertFalse(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		assertFalse(d.getBooleanAndArithmeticDiff().originalImpliesNew());
		assertFalse(d.getBooleanAndArithmeticDiff().biImplication());
		assertTrue(d.getBooleanAndArithmeticDiff().disjunction());
		
		oldCondition = "A || B";
		newCondition = "A || C";
		d = extractConditionDiff(oldCondition, newCondition);
		assertFalse(d.getBooleanAndArithmeticDiff().newImpliesOriginal());
		assertFalse(d.getBooleanAndArithmeticDiff().originalImpliesNew());
		assertFalse(d.getBooleanAndArithmeticDiff().biImplication());
		assertFalse(d.getBooleanAndArithmeticDiff().disjunction());
		assertTrue(d.getBooleanAndArithmeticDiff().intersection());
	}

	
	final String function = 
			"void f(){\n"
			+ "  if (%s) {\n"
			+ "    true();\n"
			+ "  } else {\n"
			+ "    false();\n"
			+ "  }\n"
			+ "}";
	
	
	ConditionDiff extractConditionDiff(String oldCondition, String newCondition){
		String oldTestFunction = String.format(function, oldCondition);
		String newTestFunction = String.format(function, newCondition);
		TestStringFunctionDiffParser parser = new TestStringFunctionDiffParser(oldTestFunction, newTestFunction);
		
		parser.activateVisualization();
		FunctionDiff d = parser.parse();
		
		assert(d.getOriginalFunction().getContent().getChild(0).getChild(0) instanceof Condition);
		assert(d.getNewFunction().getContent().getChild(0).getChild(0) instanceof Condition);
		Condition oldCond = (Condition) d.getOriginalFunction().getContent().getChild(0).getChild(0);
		Condition newCond = (Condition) d.getNewFunction().getContent().getChild(0).getChild(0);
		System.out.println("Original Condition: " + oldCond);
		System.out.println("New Condition:" + newCond);
		assert(d.getFineGrainedDiff().getMappedNode(oldCond).equals(newCond));
		
		return new ConditionDiff(oldCond, newCond, d.getFineGrainedDiff());
	}
	
	Condition extractCondition(String condition){
		String testFunction = String.format(function, condition);
		TestStringFunctionParser p = new TestStringFunctionParser(testFunction);
		FunctionDef f = p.parse();
		Condition c = (Condition) f.getContent().getChild(0).getChild(0);
		return c;
	}
	
}
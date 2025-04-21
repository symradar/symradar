package tools.safepatch.tests;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Test;

import ast.statements.CompoundStatement;
import ast.statements.Condition;
import ast.statements.ElseStatement;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.fgdiff.ASTDiff.ACTION_TYPE;

/**
 * @author Eric Camellini
 *
 */
public class FineGrainedDiffTest {

	String function = 
			"void f(){\n"
			+ "  if (%s) {\n"
			+ "    %s\n"
			+ "  } else {\n"
			+ "    %s\n"
			+ "  }\n"
			+ "}"; 
	
	@Test
	public void test_base() {
		String oldTestFunction = String.format(function, "condOld", "stmtTrueOld0;\nstmtTrueOld1;", "stmtFalseOld0;");
		String newTestFunction = String.format(function, "condNew", "stmtTrueOld0;", "stmtFalseOld0;\nstmtFalseNew0;");
		TestStringFunctionDiffParser parser = new TestStringFunctionDiffParser(oldTestFunction, newTestFunction);
		
		parser.activateVisualization();
		FunctionDiff d = parser.parse();
		
		assertTrue(d.getFineGrainedDiff().getMappedNode(
				d.getOriginalFunction()).equals(d.getNewFunction()));
		assertTrue(d.getFineGrainedDiff().getOriginalNodesByActionType(ACTION_TYPE.UPDATE).size() == 1);
		assertTrue(d.getFineGrainedDiff().getNewNodesByActionType(ACTION_TYPE.UPDATE).size() == 1);
		assertTrue(d.getFineGrainedDiff().getOriginalNodesByActionType(ACTION_TYPE.DELETE).size() == 2);
		assertTrue(d.getFineGrainedDiff().getNewNodesByActionType(ACTION_TYPE.INSERT).size() == 2);
		
		assertTrue(!d.getFineGrainedDiff().containsSomeActionsOfType(d.getNewFunction(), ACTION_TYPE.DELETE));
		assertTrue(!d.getFineGrainedDiff().containsSomeActionsOfType(d.getOriginalFunction(), ACTION_TYPE.INSERT));
		assertTrue(!d.getFineGrainedDiff().containsSomeActionsOfType(d.getNewFunction(), ACTION_TYPE.MOVE));
		assertTrue(!d.getFineGrainedDiff().containsSomeActionsOfType(d.getOriginalFunction(), ACTION_TYPE.MOVE));
		assertTrue(d.getFineGrainedDiff().containsSomeActionsOfType(d.getOriginalFunction(), ACTION_TYPE.UPDATE));
		assertTrue(d.getFineGrainedDiff().containsSomeActionsOfType(d.getNewFunction(), ACTION_TYPE.UPDATE));
		assertTrue(d.getFineGrainedDiff().containsSomeActionsOfType(d.getOriginalFunction(), ACTION_TYPE.DELETE));
		assertTrue(d.getFineGrainedDiff().containsSomeActionsOfType(d.getNewFunction(), ACTION_TYPE.INSERT));
		
		Condition c = (Condition) d.getNewFunction().getChild(0).getChild(0).getChild(0);
		CompoundStatement comp = (CompoundStatement) d.getNewFunction().getChild(0).getChild(0).getChild(1);
		ElseStatement elseStmt = (ElseStatement) d.getNewFunction().getChild(0).getChild(0).getChild(2);
		
		assertTrue(d.getFineGrainedDiff().containsSomeActions(c));
		assertFalse(d.getFineGrainedDiff().containsSomeActions(comp));
		assertTrue(d.getFineGrainedDiff().containsSomeActions(elseStmt));
		
		assertTrue(d.getFineGrainedDiff().containsOnlyActionsOfType(elseStmt, ACTION_TYPE.INSERT));
		assertFalse(d.getFineGrainedDiff().containsOnlyActionsOfType(elseStmt, ACTION_TYPE.UPDATE));
		assertTrue(d.getFineGrainedDiff().containsOnlyActionsOfType(c, ACTION_TYPE.UPDATE));
		assertFalse(d.getFineGrainedDiff().containsOnlyActionsOfType(comp, ACTION_TYPE.INSERT));
		
	}

}

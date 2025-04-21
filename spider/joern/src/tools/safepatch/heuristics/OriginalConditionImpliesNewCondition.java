package tools.safepatch.heuristics;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;

import ast.ASTNode;
import ast.statements.Condition;
import tools.safepatch.conditions.ConditionDiff;
import tools.safepatch.diff.ASTDelta;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.fgdiff.ASTDiff;
import tools.safepatch.fgdiff.ASTDiff.ACTION_TYPE;

/**
 * @author Eric Camellini
 * @deprecated
 */
public class OriginalConditionImpliesNewCondition extends SafepatchHeuristic{

	private HashMap types;

	public OriginalConditionImpliesNewCondition(FunctionDiff diff) {
		super(diff);
		this.types = new HashMap<>();
		this.types.putAll(this.getDiff().getOriginalTypes());
		this.types.putAll(this.getDiff().getNewTypes());
	}

	@Override
	public boolean applyOnDelta(ASTDelta d) {
		List<ConditionDiff> cDiffs = HeuristicsHelper.getModifiedConditions(d.getFineGrainedDiff(), this.types, false, false);
		return HeuristicsHelper.getOriginalImpliesNew(cDiffs).size() > 0;
	}
	
	@Override
	public boolean applyOnFunction() {
		List<ConditionDiff> cDiffs = HeuristicsHelper.getModifiedConditions(this.getDiff().getFineGrainedDiff(), this.types, false, false);
		return HeuristicsHelper.getOriginalImpliesNew(cDiffs).size() > 0;
	}

}

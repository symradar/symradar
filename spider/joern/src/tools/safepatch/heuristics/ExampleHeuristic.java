package tools.safepatch.heuristics;

import java.util.List;
import java.util.stream.Collectors;

import com.github.gumtreediff.actions.model.Action;

import ast.statements.Condition;
import ast.statements.Statement;
import sun.reflect.generics.reflectiveObjects.NotImplementedException;
import tools.safepatch.fgdiff.ASTDiff.ACTION_TYPE;
import tools.safepatch.util.DefUseUtil;
import tools.safepatch.diff.ASTDelta;
import tools.safepatch.diff.FunctionDiff;

/**
 * @author Eric Camellini
 *
 */
public class ExampleHeuristic extends SafepatchHeuristic {

	public ExampleHeuristic(FunctionDiff diff) {
		super(diff);
	}


	@Override
	public boolean applyOnDelta(ASTDelta d) {
		//If the change is read only
		if (!this.getDiff().getReadWriteDiff().isWrite(d) &&
				this.getDiff().getReadWriteDiff().isRead(d)){
			
			//If a new condition is inserted
			if (d.getFineGrainedDiff()
					.getNewNodesByActionType(ACTION_TYPE.INSERT)
					.stream()
					.anyMatch(node -> node instanceof Condition)){
				return true;
			}
		}
		
		return false;
	}


	@Override
	public boolean applyOnFunction() {
		//If the change is read only
		if (!this.getDiff().getReadWriteDiff().isWrite() &&
				this.getDiff().getReadWriteDiff().isRead()){

			//If a new condition is inserted
			if (this.getDiff().getFineGrainedDiff()
					.getNewNodesByActionType(ACTION_TYPE.INSERT)
					.stream()
					.anyMatch(node -> node instanceof Condition)){
				return true;
			}
		}

		return false;
	}


}

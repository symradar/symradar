package tools.safepatch.heuristics;

import java.util.List;
import java.util.stream.Collectors;

import com.github.gumtreediff.actions.model.Action;

import tools.safepatch.diff.ASTDelta;
import tools.safepatch.diff.FunctionDiff;

/**
 * @author Eric Camellini
 * @deprecated
 */
public abstract class SafepatchHeuristic {
	/*
	 * TODO:
	 * - When applied to the whole function it should not return
	 * 		only true or false. 
	 */
	private FunctionDiff fDiff;
	
	public SafepatchHeuristic(FunctionDiff diff) {
		this.fDiff = diff;
	}
	
	
	
	public FunctionDiff getDiff() {
		return fDiff;
	}

	
	/*
	 * Given a FunctionDiff this method must return the ASTDeltas
	 * that satisfy the heuristic (only checking the ones in the function itself).
	 */
	public List<ASTDelta> apply(){
		return fDiff.getAllDeltas()
				.stream()
				.filter(d -> this.applyOnDelta(d))
				.collect(Collectors.toList());
	}
	
	

	/*
	 * Given an ASTDelta this function returns true
	 * if the delta satisfy the heuristic.
	 */
	public abstract boolean applyOnDelta(ASTDelta d);
	
	
	/*
	 * Given a FunctionDiff this method tells if the whole function
	 * satisfies the heuristic. It is meant to be used on the function directly
	 * without passing through the coarse-grained diff.
	 */
	public abstract boolean applyOnFunction();
	
}

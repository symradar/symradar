package tools.safepatch.cfgimpl;

import ast.ASTNode;

/**
 * Constraint for cases where a variable is 
 * constrained to a single value
 */
public class SingleValueConstraint implements Constraint {
	
	// possible values that it can assume
	private ASTNode value;
	
	/**
	 * Constructor for cases when there is only one possible values
	 */
	public SingleValueConstraint(ASTNode value) {
		this.value = value;
	}

	public ASTNode getValue() {
		return value;
	}
	
	@Override
	public String toString() {
		return this.getClass().getSimpleName() + "[" + value.getEscapedCodeStr() + "]";
	}
}

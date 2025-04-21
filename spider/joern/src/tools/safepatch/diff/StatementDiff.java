/**
 * 
 */
package tools.safepatch.diff;

import ast.functionDef.FunctionDef;

/**
 * This class handles the difference between 2 functions definitions
 * at the statement level.
 * @author machiry
 *
 */
public class StatementDiff {
	private FunctionDef srcFunc = null;
	private FunctionDef dstFunc = null;

	public StatementDiff(FunctionDef sFunc, FunctionDef dFunc) {
		this.srcFunc = sFunc;
		this.dstFunc = dFunc;
	}
	
	
}

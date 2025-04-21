/**
 * 
 */
package tools.safepatch.fgdiff;

import ast.ASTNode;

/**
 * @author machiry
 *
 */
public class StatementMap {
	public enum MAP_TYPE {
        INSERT, 
        DELETE, 
        MOVE,
        UPDATE,
        UNMODIFIED,
        UNDEFINED
    }
	
	private ASTNode origStatement = null;
	private ASTNode newStatement = null;
	private MAP_TYPE targetMap = MAP_TYPE.UNDEFINED;
	
	public StatementMap(ASTNode ostmt, ASTNode nstmt, MAP_TYPE currType) {
		this.origStatement = ostmt;
		this.newStatement = nstmt;
		this.targetMap = currType;
	}
	
	public ASTNode getOriginal() {
		return this.origStatement;
	}
	
	public ASTNode getNew() {
		return this.newStatement;
	}
	
	public MAP_TYPE getMapType() {
		return this.targetMap;
	}
	
	@Override
	public int hashCode() {
		int retVal = 0;
		if(origStatement != null) {
			retVal ^= origStatement.hashCode();
		}
		if(newStatement != null) {
			retVal ^= newStatement.hashCode();
		}
		return retVal;
	}
	
	@Override
	public boolean equals(Object o) {
		boolean retVal = false;
		if(o != null && o instanceof StatementMap) {
			StatementMap that = (StatementMap)o;
			if(this.origStatement == null) {
				retVal = this.origStatement == that.origStatement;
			} else {
				retVal = this.origStatement.equals(that.origStatement);
			}
			if(this.newStatement == null) {
				retVal &= this.newStatement == that.newStatement;
			} else {
				retVal &= this.newStatement.equals(that.newStatement);
			}
		}
		return retVal;
	}
}

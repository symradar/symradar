package ast;

import org.antlr.v4.runtime.ParserRuleContext;
import org.hamcrest.core.IsInstanceOf;

public class CodeLocation implements Comparable<CodeLocation>
{

	final private int NOT_SET = -1;

	public int startLine = NOT_SET;
	public int startPos = NOT_SET;
	public int startIndex = NOT_SET;
	public int stopIndex = NOT_SET;
	public int stopLine = NOT_SET;
	
	public CodeLocation()
	{
	}

	public CodeLocation(ParserRuleContext ctx)
	{
		initializeFromContext(ctx);
	}

	private void initializeFromContext(ParserRuleContext ctx)
	{
		startLine = ctx.start.getLine();
		startPos = ctx.start.getCharPositionInLine();
		startIndex = ctx.start.getStartIndex();
		if (ctx.stop != null){
			stopIndex = ctx.stop.getStopIndex();
			stopLine = ctx.stop.getLine();
		} else {
			stopIndex = NOT_SET;
			stopLine = NOT_SET;
		}
			
	}

	@Override
	public String toString()
	{
		return String.format("%d:%d:%d:%d:%d", startLine, startPos, stopLine, 
				startIndex, stopIndex);
	}
	
	@Override
	public int hashCode() {
		return startLine ^ startPos ^ startIndex ^ stopIndex ^ stopLine;
	}
	
	@Override
	public boolean equals(Object that) {
		boolean retVal = false;
		if(that instanceof CodeLocation) {
			CodeLocation newObj = (CodeLocation)that;
			retVal = newObj.startIndex == this.startIndex && 
					newObj.startLine == this.startLine && 
					newObj.startPos == this.startPos && 
					newObj.stopIndex == this.stopIndex && 
					newObj.stopLine == this.stopLine;
		}
		return retVal;
	}

	@Override
	public int compareTo(CodeLocation o) {
		/*
		 * TODO:
		 * - Check if this is ok
		 */
		
		if(this.startLine == o.startLine) {
			return this.startIndex - o.startIndex;
		}
		return this.startLine - o.startLine;
	}

}

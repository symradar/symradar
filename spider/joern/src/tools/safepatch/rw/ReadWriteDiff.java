package tools.safepatch.rw;

import java.util.List;

import tools.safepatch.diff.ASTDelta;
import tools.safepatch.diff.FunctionDiff;
import udg.symbols.UseOrDefSymbol;

/**
 * @author Eric Camellini
 *
 */
public class ReadWriteDiff {

	FunctionDiff diff;

	public ReadWriteDiff(FunctionDiff diff) {
		this.diff = diff;
	}
	
	public List<UseOrDefSymbol> insertedReads(){
		return ReadWriteUtil.insertedReads(null, diff);
	}
	
	public List<UseOrDefSymbol> insertedWrites(){
		return ReadWriteUtil.insertedWrites(null, diff);
	}
	
	public List<UseOrDefSymbol> deletedReads(){
		return ReadWriteUtil.deletedReads(null, diff);
	}
	
	public List<UseOrDefSymbol> deletedWrites(){
		return ReadWriteUtil.deletedWrites(null, diff);
	}
	
	public List<UseOrDefSymbol> insertedReads(ASTDelta d){
		return ReadWriteUtil.insertedReads(d, diff);
	}
	
	public List<UseOrDefSymbol> insertedWrites(ASTDelta d){
		return ReadWriteUtil.insertedWrites(d, diff);
	}
	
	public List<UseOrDefSymbol> deletedReads(ASTDelta d){
		return ReadWriteUtil.deletedReads(d, diff);
	}
	
	public List<UseOrDefSymbol> deletedWrites(ASTDelta d){
		return ReadWriteUtil.deletedWrites(d, diff);
	}
	
	public boolean isRead(){
		return ((this.insertedReads().size() != 0) ||
				(this.deletedReads().size() != 0));
	}
	
	public boolean isWrite(){
		return ((this.insertedWrites().size() != 0) ||
				(this.deletedWrites().size() != 0));
	}
	
	public boolean isRead(ASTDelta d){
		return ((this.insertedReads(d).size() != 0) ||
				(this.deletedReads(d).size() != 0));
	}
	
	public boolean isWrite(ASTDelta d){
		return ((this.insertedWrites(d).size() != 0) ||
				(this.deletedWrites(d).size() != 0));
	}
	
}

package tools.safepatch.diff;
import java.util.ArrayList;
import java.util.List;

import ast.statements.CompoundStatement;
import ast.statements.Statement;
import difflib.Chunk;
import tools.safepatch.fgdiff.ASTDiff;
import tools.safepatch.fgdiff.GumtreeASTMapper;

/**
 * @author Eric Camellini
 * 
 * Coarse-grained delta (Chunk level, with corresponding statements). 
 */
public class ASTDelta {

	public enum DELTA_TYPE {
    	/** A change in the original. */
        CHANGE, 
        /** A delete from the original. */
        DELETE, 
        /** An insert into the original. */
        INSERT
    }
	
	private List<Statement> originalStatements;
	private List<Statement> newStatements;
	private Chunk<String> originalChunk;
	private Chunk<String> newChunk;
	private DELTA_TYPE type;
	private ASTDiff fineGrainedDiff = null;
	
	public List<Statement> getOriginalStatements() {
		return originalStatements;
	}

	public List<Statement> getNewStatements() {
		return newStatements;
	}

	
	public Chunk<String> getOriginalChunk() {
		return originalChunk;
	}

	public Chunk<String> getNewChunk() {
		return newChunk;
	}

	public ASTDelta(Chunk<String> originalChunk, Chunk<String> newChunk, 
			List<Statement> originalStatements, List<Statement> newStatements,
			DELTA_TYPE type) {
		this.originalChunk = originalChunk;
		this.newChunk = newChunk;
		this.originalStatements = originalStatements;
		this.newStatements = newStatements;
		this.type = type;
	}

	public DELTA_TYPE getType(){
		return this.type;
	}
	
	/**
	 * Generate Gumtree fine grained diff for these two chunks,
	 * with mappings between old and new nodes and between
	 * Joern and Gumtree ASTs
	 */
	public void generateFineGrainedDiff(){
		if(this.fineGrainedDiff == null){
			GumtreeASTMapper mapper = new GumtreeASTMapper();
			if(this.type.equals(DELTA_TYPE.CHANGE))
				this.fineGrainedDiff = mapper.map(this.originalStatements, this.newStatements);
			else if(this.type.equals(DELTA_TYPE.INSERT))
				this.fineGrainedDiff = mapper.map(new ArrayList<Statement>(), this.newStatements);
			else if(this.type.equals(DELTA_TYPE.DELETE))
				this.fineGrainedDiff = mapper.map(this.originalStatements, new ArrayList<Statement>());
		}
	}
	
	
	
	public ASTDiff getFineGrainedDiff() {
		return fineGrainedDiff;
	}
	
}

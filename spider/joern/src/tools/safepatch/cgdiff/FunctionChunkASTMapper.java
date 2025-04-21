package tools.safepatch.cgdiff;

import java.util.ArrayList;
import java.util.List;
import java.util.Observer;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ast.ASTNode;
import ast.functionDef.FunctionDef;
import ast.statements.BlockStarter;
import ast.statements.CompoundStatement;
import ast.statements.DoStatement;
import ast.statements.ElseStatement;
import ast.statements.ForStatement;
import ast.statements.IfStatement;
import ast.statements.Statement;
import ast.statements.SwitchStatement;
import ast.statements.WhileStatement;
import difflib.Chunk;

/**
 * @author Eric Camellini
 * @see https://www.adictosaltrabajo.com/tutoriales/comparar-ficheros-java-diff-utils/
 * 
 */
public class FunctionChunkASTMapper {
	
	private static final Logger logger = LogManager.getLogger();
	
	private enum STATUS {
		BEFORE_CHUNK,
		INSIDE_CHUNK,
		AFTER_CHUNK
	}
	
	private FunctionDef function;
	private Chunk<String> deltaChunk;
	int off;
	int chunkStart;
	int chunkEnd;
	int chunkSize;
	
	private ArrayList<Statement> affectedStatements;
	
	STATUS status;
	
	public FunctionChunkASTMapper(FunctionDef function, Chunk<String> deltaChunk) {
		this.function = function;
		this.deltaChunk = deltaChunk;
		this.off = this.function.getContent().getLocation().startLine - 1;
		
		this.chunkSize = this.deltaChunk.getLines().size();
		this.chunkStart = this.deltaChunk.getPosition() + 1;
		this.chunkEnd = chunkStart + chunkSize - 1;
	}
	
	
	public FunctionDef getFunction() {
		return function;
	}
	
	public ArrayList<Statement> getChunkStatements() {
		return affectedStatements;
	}
	
	
	public void diff(){
		this.affectedStatements = new ArrayList<Statement>();
		this.status = STATUS.BEFORE_CHUNK;
		if(logger.isDebugEnabled()) logger.debug("Delta start: " + chunkStart + " - Delta end: " + chunkEnd);
		if(logger.isDebugEnabled()) logger.debug("Looking for corresponding statements in function:\n");
		//printFunction();
		if(logger.isDebugEnabled()) logger.debug("FUNCTION START");
		helper(function.getContent());
		if(logger.isDebugEnabled()) logger.debug("FUNCTION END");
	}
	
	private void helper(ASTNode node){
		/*
		 * TODO: Re-design algorithm in a better way
		 *
		 */
		if(logger.isDebugEnabled()) logger.debug("");
		if(logger.isDebugEnabled()) logger.debug("Handling node:");
		if(logger.isDebugEnabled()) logger.debug(node);
		//printChild(node);
		if(this.status == STATUS.AFTER_CHUNK){
			if(logger.isDebugEnabled()) logger.debug("This statement is {}", this.status);
			return;
		}

		if(!(node instanceof Statement))
			return;
		Statement stmt = (Statement) node;

		if(stmt instanceof CompoundStatement){
			if(logger.isDebugEnabled()) logger.debug("{");			
			List<ASTNode> childs = ((CompoundStatement) stmt).getStatements();
			for (ASTNode child : childs)
				helper(child);
			if(logger.isDebugEnabled()) logger.debug("}");
			return;
		}

		this.updateStatus(stmt);
		if(logger.isDebugEnabled()) logger.debug("This statement is {}", this.status);

		if(this.status == STATUS.INSIDE_CHUNK){
			if(logger.isDebugEnabled()) logger.debug("Added to affected statements");
			/*
			 * Remove the if to consider ElseStatement separately from the corresponding If 
			 */
			if(!(stmt instanceof ElseStatement)){
				this.affectedStatements.add(stmt);
			} else {
				if (!affectedStatements.contains(((ElseStatement) stmt).getIfNode()))
					affectedStatements.add(((ElseStatement) stmt).getIfNode());
			}
		}

		if(stmt instanceof IfStatement){
			IfStatement ifStmt = (IfStatement) stmt;

			//Handle then block
			if(this.status ==  STATUS.BEFORE_CHUNK){
				if(logger.isDebugEnabled()) logger.debug("THEN START");
				helper(ifStmt.getChild(1)); 
				if(logger.isDebugEnabled()) logger.debug("THEN END");
			}

			//Handle else block
			if (ifStmt.getElseNode() != null)
				helper(ifStmt.getElseNode());
			return;
		}

		if(stmt instanceof BlockStarter && 
				!(stmt instanceof IfStatement) &&
				this.status == STATUS.BEFORE_CHUNK){

			if(stmt instanceof ElseStatement){
				ElseStatement elseStmt = (ElseStatement) stmt;

				if(logger.isDebugEnabled()) logger.debug("ELSE START");
				helper(elseStmt.getChild(0));
				if(logger.isDebugEnabled()) logger.debug("ELSE END");

				return;
			}

			if(stmt instanceof DoStatement){
				DoStatement doStmt = (DoStatement) stmt;

				if(logger.isDebugEnabled()) logger.debug("DO WHILE START");
				helper(doStmt.getChild(0));
				//printChild(doStmt.getChild(1));
				if(logger.isDebugEnabled()) logger.debug("DO WHILE END");

				return;
			}

			if(stmt instanceof ForStatement){
				ForStatement forStmt = (ForStatement) stmt;
				
				if(logger.isDebugEnabled()) logger.debug("FOR START");
				helper(forStmt.getChild(forStmt.getChildCount() - 1));
				if(logger.isDebugEnabled()) logger.debug("FOR END");

				return;
			}

			if(stmt instanceof WhileStatement){
				WhileStatement whileStmt = (WhileStatement) stmt;
	
				if(logger.isDebugEnabled()) logger.debug("WHILE START");
				helper(whileStmt.getChild(1));
				if(logger.isDebugEnabled()) logger.debug("WHILE END");

				return;
			}

			if(stmt instanceof SwitchStatement){
				SwitchStatement switchStmt = (SwitchStatement) stmt;

				if(logger.isDebugEnabled()) logger.debug("SWITCH START");
				helper(switchStmt.getChild(1));
				if(logger.isDebugEnabled()) logger.debug("SWITCH END");

				return;
			}

			//TODO: (?) Add try and catch to support C++
		}

		return;

	}

	private void updateStatus(Statement stmt){
		if(getStartLine(stmt) <= chunkStart && 
				getEndLine(stmt) >= chunkStart && 
				this.status == STATUS.BEFORE_CHUNK){
			if(logger.isDebugEnabled()) logger.debug("BEFORE CHUNK ==> INSIDE CHUNK (1)");
			this.status = STATUS.INSIDE_CHUNK;
			return;
		}

		if(getStartLine(stmt) > chunkStart &&
				getStartLine(stmt) <= chunkEnd &&
				this.status == STATUS.BEFORE_CHUNK){
			if(logger.isDebugEnabled()) logger.debug("BEFORE CHUNK ==> INSIDE CHUNK (2)");
			this.status = STATUS.INSIDE_CHUNK;
			return;
		}

		if(getStartLine(stmt) > chunkEnd &&
				(this.status == STATUS.INSIDE_CHUNK || this.status == STATUS.BEFORE_CHUNK)){
			if(logger.isDebugEnabled()) logger.debug("INSIDE CHUNK ==> AFTER CHUNK (1)");
			this.status = STATUS.AFTER_CHUNK;
			return;
		}
	}

	private int getStartLine(ASTNode node){
		return node.getLocation().startLine + off;
	}
	
	private int getEndLine(ASTNode node){
		return node.getLocation().stopLine + off;
	}
	
//	private void printChild(ASTNode node){
//		if(logger.isDebugEnabled()) if(logger.isDebugEnabled()) logger.debug(node + 
//				" Start: " + getStartLine(node) +
//				" End: " + getEndLine(node) +
//				" - Code: " + node.getEscapedCodeStr()
//				);
//	}
//
//	private void printFunction(){
//		if(logger.isDebugEnabled()) if(logger.isDebugEnabled()) logger.debug(function + 
//				//" " +  node.getLocation() +
//				" Start: " + (function.getLocation().startLine) +
//				" End: " + (function.getLocation().stopLine) +
//				" - Code: " + function.getEscapedCodeStr()
//				);
//	}

}

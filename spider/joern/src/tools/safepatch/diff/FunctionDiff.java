package tools.safepatch.diff;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import javax.sound.midi.Synthesizer;

import org.antlr.v4.runtime.tree.ParseTreeVisitor;
import org.antlr.v4.runtime.tree.ParseTreeWalker;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.sun.org.apache.xalan.internal.xsltc.compiler.Parser;

import ast.ASTNode;
import ast.expressions.AssignmentExpr;
import ast.functionDef.FunctionDef;
import ast.statements.CompoundStatement;
import ast.statements.Statement;
import cfg.C.CCFG;
import ddg.DataDependenceGraph.DDGDifference;
import difflib.Chunk;
import difflib.Delta;
import parsing.ParseTreeUtils;
import tools.safepatch.cgdiff.FileDiff;
import tools.safepatch.cgdiff.FunctionChunkASTMapper;
import tools.safepatch.fgdiff.ASTDiff;
import tools.safepatch.fgdiff.GumtreeASTMapper;
import tools.safepatch.rw.ASTToRWTableConverter;
import tools.safepatch.rw.ReadWriteDiff;
import tools.safepatch.rw.ReadWriteTable;
import tools.safepatch.util.AffectedVariablesUtil;
import tools.safepatch.util.BasicBlock;
import tools.safepatch.util.BasicBlocksExtractor;
import tools.safepatch.util.TypeInference;

/**
 * @author Eric Camellini
 * 
 */
public class FunctionDiff {
	
	private static final Logger logger = LogManager.getLogger();
	
	private File oldFile;
	private File newFile;

	private FunctionDef originalFunction;
	private FunctionDef newFunction;

	private ArrayList<ASTDelta> astDeltas;
	
	private JoernGraphs originalGraphs = null;
	private JoernGraphs newGraphs = null;
	private DDGDifference ddgDiff = null;
	
	private ReadWriteTable originalReadWriteTable = null;
	private ReadWriteTable newReadWriteTable = null;
	private ReadWriteDiff readWriteDiff = null;

	private ASTDiff functionFineGrainedDiff;

	private Map<String, String> originalTypes = null;
	private Map<String, String> newTypes = null;
	
	private List<BasicBlock> originalBasicBlocks = null;
	private List<BasicBlock> newBasicBlocks = null;
	
	public ASTDiff getFineGrainedDiff() {
		return functionFineGrainedDiff;
	}

	public void typeInference(){
		if(this.originalTypes == null)
			this.originalTypes = TypeInference.getTypeInference(this.originalFunction);
		if(this.newTypes == null)
			this.newTypes = TypeInference.getTypeInference(this.newFunction);
	}
	
	public FunctionDiff(File oldFile, FunctionDef originalFunction, File newFile, FunctionDef newFunction) {
		super();
		this.originalFunction = originalFunction;
		this.newFunction = newFunction;
		this.newFile = newFile;
		this.oldFile = oldFile;
	}

	public void generateGraphs(){
		if(this.originalGraphs == null)
			this.originalGraphs = new JoernGraphs(this.originalFunction);
		
		if(this.newGraphs == null)
			this.newGraphs = new JoernGraphs(this.newFunction);
		
		if(this.ddgDiff == null)
			this.ddgDiff = this.originalGraphs.getDdg().difference(
					this.newGraphs.getDdg());
	}
	
	public void generateReadWriteInformation(){
		if(this.originalReadWriteTable == null){
			ASTToRWTableConverter c = new ASTToRWTableConverter();
			this.originalReadWriteTable = c.convert(originalFunction);
		}
		
		if(this.newReadWriteTable == null){
			ASTToRWTableConverter c = new ASTToRWTableConverter();
			this.newReadWriteTable = c.convert(newFunction);
		}
		
		if(this.readWriteDiff == null){
			readWriteDiff = new ReadWriteDiff(this);
		}
	}
	
	public FunctionDef getOriginalFunction() {
		return originalFunction;
	}

	public FunctionDef getNewFunction() {
		return newFunction;
	}
	
	public File getOldFile() {
		return oldFile;
	}

	public void setOldFile(File oldFile) {
		this.oldFile = oldFile;
	}

	public File getNewFile() {
		return newFile;
	}

	public void setNewFile(File newFile) {
		this.newFile = newFile;
	}

	public Map<String, String> getOriginalTypes() {
		return originalTypes;
	}

	public Map<String, String> getNewTypes() {
		return newTypes;
	}

	public List<ASTDelta> getAllDeltas(){
		return this.astDeltas;
	}
	
	public List<ASTDelta> getDeltas(ASTDelta.DELTA_TYPE type){
		switch (type) {
		case INSERT:
			return getInsertions();

		case DELETE:
			return getDeletes();
		
		case CHANGE:
			return getChanges();
			
		default:
			throw new RuntimeException("Unknown DELTA_TYPE.");
		}
	}
	
	private List<ASTDelta> getInsertions() {
		return this.astDeltas.stream()
				.filter(p -> p.getType() == ASTDelta.DELTA_TYPE.INSERT).collect(Collectors.toList());
	}

	private List<ASTDelta> getDeletes() {
		return this.astDeltas.stream()
				.filter(p -> p.getType() == ASTDelta.DELTA_TYPE.DELETE).collect(Collectors.toList());
	}

	private List<ASTDelta> getChanges() {
		return this.astDeltas.stream()
				.filter(p -> p.getType() == ASTDelta.DELTA_TYPE.CHANGE).collect(Collectors.toList());
	}

	
	public JoernGraphs getOriginalGraphs() {
		return originalGraphs;
	}

	public JoernGraphs getNewGraphs() {
		return newGraphs;
	}

	public DDGDifference getDdgDiff() {
		return ddgDiff;
	}

	public ReadWriteTable getOriginalReadWriteTable() {
		return originalReadWriteTable;
	}

	public ReadWriteTable getNewReadWriteTable() {
		return newReadWriteTable;
	}
	
	public ReadWriteDiff getReadWriteDiff() {
		return readWriteDiff;
	}
	
	
	public List<BasicBlock> getOriginalBasicBlocks() {
		if(originalBasicBlocks == null){
			if(logger.isDebugEnabled()) logger.debug("Extracting original basic blocks...");
			BasicBlocksExtractor e = new BasicBlocksExtractor();
			this.originalBasicBlocks = e.getBasicBlocks((CCFG) this.getOriginalGraphs().getCfg(), this.getOriginalGraphs().getCdg());
			if(logger.isDebugEnabled()) logger.debug("Original basic blocks extracted.");
		}
			
		return originalBasicBlocks;
	}


	public List<BasicBlock> getNewBasicBlocks() {
		if(newBasicBlocks == null){
			if(logger.isDebugEnabled()) logger.debug("Extracting new basic blocks...");
			BasicBlocksExtractor e = new BasicBlocksExtractor();
			this.newBasicBlocks = e.getBasicBlocks((CCFG) this.getNewGraphs().getCfg(), this.getNewGraphs().getCdg());
			if(logger.isDebugEnabled()) logger.debug("New basic blocks extracted.");
		}
		
		return newBasicBlocks;
	}


	/**
	 * Coarse-grained diff.
	 * TODO 
	 * fix this:
	 * The problem is in some CVEs
	 * e.g. CVE-2015-1283
	 */
	public void generateCoarseGrainedDiff(FileDiff diff){
		this.astDeltas = new ArrayList<ASTDelta>();

		if (!diff.getOriginal().equals(this.oldFile) ||
				!diff.getRevised().equals(this.newFile))
			throw new RuntimeException("Diff must be on the same files from which the two versions of the common function are extracted.");

		for (Delta<String> d : diff.getAllDeltas()){
			Chunk<String> rev = d.getRevised();
			Chunk<String> orig = d.getOriginal();
			if(logger.isDebugEnabled()) logger.debug("Orig chunk: {}", orig);
			if(logger.isDebugEnabled()) logger.debug("Revised chunk: {}", rev);

			//			if(this.astDeltas.stream().anyMatch(de -> 
			//			((de.getOriginalChunk() != null && de.getOriginalChunk().equals(orig)) || 
			//					(de.getNewChunk() != null && de.getNewChunk().equals(rev)))))
			//				//Already added
			//				continue;
			//			else{

			switch (d.getType()) {
			/*TODO:
			 * - Untangle code changes with state of the art approaches ?
			 * ___________________________________________________________
			 * 
			 * This approach could be improved because at the moment it
			 * cannot handle deltas that spread over multiple functions, but
			 * only the ones in which both chunks (old and new) are contained
			 * within the two functions.
			 * 
			 * Idea to do it:
			 * - For every delta go through every line and find the corresponding function
			 * - Is it really needed?
			 */

			case INSERT:
				if(chunkInFunction(rev, this.newFunction)){
					this.astDeltas.add(new ASTDelta(null, rev,
							null, astChunkDiff(this.newFunction, rev), ASTDelta.DELTA_TYPE.INSERT));
					if(logger.isDebugEnabled()) logger.debug("INSERTION:");
					if(logger.isDebugEnabled()) logger.debug("\n" + FileDiff.linesToString(d.getRevised().getLines()));

				}
				break;

			case DELETE:
				if(chunkInFunction(orig, this.originalFunction)){
					this.astDeltas.add(new ASTDelta(orig, null,
							astChunkDiff(this.originalFunction, orig), null, ASTDelta.DELTA_TYPE.DELETE));
					if(logger.isDebugEnabled()) logger.debug("DELETE:");
					if(logger.isDebugEnabled()) logger.debug("\n" + FileDiff.linesToString(d.getOriginal().getLines()));

				}
				break;

			case CHANGE:
				if (chunkInFunction(rev, this.newFunction) &&
						chunkInFunction(orig, this.originalFunction)){
					if(logger.isDebugEnabled()) logger.debug("CHANGE---:");
					if(logger.isDebugEnabled()) logger.debug("\n" + FileDiff.linesToString(d.getOriginal().getLines()));

					if(logger.isDebugEnabled()) logger.debug("CHANGE+++:\n");
					if(logger.isDebugEnabled()) logger.debug("\n" + FileDiff.linesToString(d.getRevised().getLines()));
					this.astDeltas.add(new ASTDelta(orig, rev,
							astChunkDiff(this.originalFunction, orig), astChunkDiff(this.newFunction, rev), ASTDelta.DELTA_TYPE.CHANGE));

				}
				break;

			default:
				break;
			}
		}
	}

	
	public static boolean chunkInFunction(Chunk<String> deltaChunk, FunctionDef func){
		if (deltaChunk.getPosition() + 1 > func.getLocation().startLine && 
				deltaChunk.getPosition() + deltaChunk.getLines().size() <= func.getLocation().stopLine )
			return true;
		else
			return false;
	}
	
	private static ArrayList<Statement> astChunkDiff(FunctionDef function, Chunk<String> deltaChunk){
		FunctionChunkASTMapper differ = new FunctionChunkASTMapper(function, deltaChunk);
		differ.diff();
		return differ.getChunkStatements();
	}
	
	
	/*
	 * Generate Gumtree fine grained diff for the whole function,
	 * with mappings between old and new nodes and between
	 * Joern and Gumtree ASTs
	 */
	public void generateFineGrainedDiff(){
		GumtreeASTMapper mapper = new GumtreeASTMapper();
		this.functionFineGrainedDiff = mapper.map(this.originalFunction, this.newFunction);
	}
	
	/**
	 * Returns from the original file all the variables whose value is affected by some changes.
	 * A variable is affected by a change if it is on the left of an AssignmentExpression
	 * that is affected by some changes, or if it's value depends in some way from 
	 * another affected variable.
	 * Only data dependencies are considered at the moment. 
	 */
	public Set<String> getOriginalAffectedVars(){
		return this.getAffectedVars(NEW_OR_ORIGINAL.ORIGINAL);
	}
	
	/**
	 * Returns from the new file all the variables whose value is affected by some changes.
	 * A variable is affected by a change if it is on the left of an AssignmentExpression
	 * that is affected by some changes, or if it's value depends in some way from 
	 * another affected variable.
	 * Only data dependencies are considered at the moment. 
	 */
	public Set<String> getNewAffectedVars(){
		return this.getAffectedVars(NEW_OR_ORIGINAL.NEW);
	}
	
	private enum NEW_OR_ORIGINAL {
		NEW,
		ORIGINAL
	}
	
	
	private Set<String> getAffectedVars(NEW_OR_ORIGINAL newOrOriginal){
		//TODO Add control dependencies
		
		if(logger.isDebugEnabled()) logger.debug("Getting affected {} variables: ", newOrOriginal);
		
		//Getting AssignmentExpressions affected by changes
		List<ASTNode> affectedAssignments = (newOrOriginal == NEW_OR_ORIGINAL.ORIGINAL ? 
				this.functionFineGrainedDiff.getOriginalAffectedNodes(AssignmentExpr.class) :
					this.functionFineGrainedDiff.getNewAffectedNodes(AssignmentExpr.class));
		if(logger.isDebugEnabled()) logger.debug("Affected assignments: {}", affectedAssignments);
		
		Set<String> affectedVars = new HashSet<String>();
		
		for(ASTNode assignment : affectedAssignments){
			//The identifiers on the left of the = are affected by the change
			affectedVars.add(assignment.getChild(0).getEscapedCodeStr());
		}
		
		if(logger.isDebugEnabled()) logger.debug("Vars: {}", affectedVars);
		
		affectedVars = (Set<String>) AffectedVariablesUtil.getAffectedVars(affectedVars,
				(newOrOriginal == NEW_OR_ORIGINAL.NEW ? this.getNewGraphs().getCfg() : 
					this.getOriginalGraphs().getCfg()),
				(newOrOriginal == NEW_OR_ORIGINAL.NEW ? this.getNewGraphs().getDefUseCfg() : 
					this.getOriginalGraphs().getDefUseCfg()));
		
		if(logger.isDebugEnabled()) logger.debug("Vars after closure: {}", affectedVars);
		return affectedVars;
	}
	
	
}


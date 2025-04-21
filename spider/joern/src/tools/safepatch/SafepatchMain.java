/**
 * 
 */
package tools.safepatch;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import org.apache.commons.cli.ParseException;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGEntryNode;
import cfg.nodes.CFGNode;

import com.fasterxml.jackson.databind.ObjectMapper;

import ast.ASTNode;
import ast.functionDef.FunctionDef;
import ast.statements.Statement;
import fileWalker.FileNameMatcher;
import parsing.ModuleParser;
import parsing.C.Modules.ANTLRCModuleParserDriver;
import tools.safepatch.FunctionMatcher.MATCHING_METHOD;
import tools.safepatch.HeuristicsDispatcher.HEURISTIC;
import tools.safepatch.HeuristicsDispatcher.PATCH_FEATURE;
import tools.safepatch.SafepatchCmdInterface.OPTION;
import tools.safepatch.SafepatchPreprocessor.PREPROCESSING_METHOD;
import tools.safepatch.cfgimpl.ASTCFGMapping;
import tools.safepatch.cgdiff.FileDiff;
import tools.safepatch.diff.ASTDelta;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.diff.JoernGraphs;
import tools.safepatch.fgdiff.GumtreeASTMapper;
import tools.safepatch.fgdiff.StatementMap;
import tools.safepatch.rw.ReadWriteTable;
import tools.safepatch.visualization.GraphvizGenerator;
import tools.safepatch.visualization.GraphvizGenerator.FILE_EXTENSION;
import udg.symbols.UseOrDefSymbol;

/**
 * @author machiry
 * @author Eric Camellini
 *
 */
public class SafepatchMain {

	private static final Logger logger = LogManager.getLogger();
	
	final static String C_FILENAME_FILTER = "*.c";
	public static final String PREPROCESSED_FILE_PREFIX = "preprocessed_";
	
	private static SafepatchCmdInterface cmd = new SafepatchCmdInterface();
	
	static FileNameMatcher matcher = new FileNameMatcher();

	static ANTLRCModuleParserDriver driver = new ANTLRCModuleParserDriver();
	static ModuleParser parser = new ModuleParser(driver);
	static SafepatchASTWalker astWalker = new SafepatchASTWalker();

	//static GumtreeASTMapper gumtreeAstMapper = new GumtreeASTMapper();
	
	static boolean visualizationOn = false;
	
	static PREPROCESSING_METHOD preprocessingMethod = PREPROCESSING_METHOD.UNIFDEFALL;
	static MATCHING_METHOD functionMatchingMethod = MATCHING_METHOD.NAME_ONLY;

	private static boolean coarseGrainedDiffEnabled = true;
	private static boolean printCoarseGrainedDiff = false;
	static boolean systemOuts = false;

	private static String output = null;
	
	static List<FunctionResult> results = new ArrayList<FunctionResult>();
	static class FunctionResult{
		public String function;
		public Integer originalLine;
		public Integer newLine;
		public Map<HEURISTIC, Boolean> heuristics;
		public Map<PATCH_FEATURE, Integer> features;
		public boolean security;
		public FunctionResult(String function,
				Map<HEURISTIC, Boolean> heuristics,
				Map<PATCH_FEATURE, Integer> features,
				boolean security,
				int originalLine,
				int newLine) {
			super();
			this.function = function;
			this.heuristics = heuristics;
			this.features = features;
			this.security = security;
			this.originalLine = originalLine;
			this.newLine = newLine;
		}
		
	}
	
	public static void main(String[] args)
	{
		
		// Reading arguments
		parseCommandLine(args);
		checkCmdOptions();
		String[] fileNames = cmd.getFilenames();
		
		matcher.setFilenameFilter(C_FILENAME_FILTER);
		parser.addObserver(astWalker);
		
		// Gumtree parameters
//	    gumtreeAstMapper.setTopDownMinHeight(2);
//		gumtreeAstMapper.setBottomUpSimilarity(0.2);
//		gumtreeAstMapper.setBottomUpMaxSize(100);
//		gumtreeAstMapper.setXYSimilarity(0.5);
		
		String oldFileName = fileNames[0];
		String newFileName = fileNames[1];
		if(systemOuts) System.out.println("Oldfile: " + oldFileName);
		if(systemOuts) System.out.println("Newfile: " + newFileName);
		String oldPreprocessed;
		String newPreprocessed;
		File oldFile = new File(oldFileName);
		File newFile = new File(newFileName);
		Path oldPath = oldFile.toPath();
		Path newPath = newFile.toPath();

		// Checking file format
		if(!matcher.fileMatches(oldPath) || !matcher.fileMatches(newPath))
			throw new RuntimeException("Old and new files must be .c source files."); //Maybe we'll add .cpp

		if(oldPath.toString().startsWith(PREPROCESSED_FILE_PREFIX) ||
				newPath.toString().startsWith(PREPROCESSED_FILE_PREFIX))
			throw new RuntimeException("Preprocessed file should not be the input.");
		
		try {
			/*
			 * TODO
			 * - Instead of preprocessing step, modify parsers? http://joern.readthedocs.io/en/latest/development.html
			 */
			
			//Pre-processing step to handle the c preprocessor primitives
			if(systemOuts) System.out.println("Preprocessing...");
			SafepatchPreprocessor prep = new SafepatchPreprocessor(preprocessingMethod);
			oldPreprocessed = oldPath.toString().replaceAll(oldPath.getFileName().toString(), 
					PREPROCESSED_FILE_PREFIX + oldPath.getFileName());
			newPreprocessed = newPath.toString().replaceAll(newPath.getFileName().toString(),
					PREPROCESSED_FILE_PREFIX + newPath.getFileName());
			prep.preProcess(oldPath.toString(), oldPreprocessed);
			prep.preProcess(newPath.toString(), newPreprocessed);
			File oldPreprocessedFile = new File(oldPreprocessed);
			File newPreprocessedFile = new File(newPreprocessed);
			
			//Parsing the preprocessed files
			if(systemOuts) System.out.println("Parsing files...");
			parser.parseFile(oldPreprocessed);
			parser.parseFile(newPreprocessed);
			
			//After parsing a file the SafepatchASTWalker saves a list of the function AST nodes in a Map,
			//for every parsed file.
			ArrayList<FunctionDef> oldFunctionList = astWalker.getFileFunctionList().get(oldPreprocessed);
			ArrayList<FunctionDef> newFunctionList = astWalker.getFileFunctionList().get(newPreprocessed);
			

			/*
			 * This is to check that there are no functions with the same name in the same file.
			 * (Can be one of the results when handling c preprocessor in a bad way or when not
			 * handling it)
			 */
			if(containsDuplicates(oldFunctionList) ||
					containsDuplicates(newFunctionList)){
				Files.delete(Paths.get(oldPreprocessed));
				Files.delete(Paths.get(newPreprocessed));
				throw new RuntimeException("Function defined two times with the same name, "
						+ "better matching method not yet implemented.");
			}
			
			/*
			 * If the functions count is different between the two files it means that one
			 * or more functions were completely deleted/inserted by the patch.
			 * We ignore these cases. 
			 * TODO this should be implemented in a different way, adding a FunctionDiff
			 * with one of the two FuncionDef objects left empty (just new FunctionDef()) 
			 */
			if(oldFunctionList.size() != newFunctionList.size()){
				Files.delete(Paths.get(oldPreprocessed));
				Files.delete(Paths.get(newPreprocessed));
				throw new RuntimeException("Functions count different between the two files: " + 
						"some functions are fully deleted/inserted.");
			}
			
			/*
			 * Even if the function count is equal, we ignore the patches
			 * where there are renamed functions (one or more).
			 */
			if(functionRenamed(oldFunctionList, newFunctionList)){
				Files.delete(Paths.get(oldPreprocessed));
				Files.delete(Paths.get(newPreprocessed));
				throw new RuntimeException("One or more functions were renamed.");
			}
			
			FileDiff diff = new FileDiff(oldPreprocessedFile, newPreprocessedFile);
			if(coarseGrainedDiffEnabled){
				//Extracting text diff
				if(systemOuts) System.out.println("Extracting textual diff from the two files...");
				diff.diff();
				if(systemOuts) System.out.println();
			}
			
			//Finding common functions (functions present in both files) according to the selected
			//matching method
			if(systemOuts) System.out.println("Finding common functions...");
			if(systemOuts) System.out.println();
			FunctionMatcher matcher = new FunctionMatcher(oldFunctionList, newFunctionList, functionMatchingMethod);
			matcher.match();
			List<FunctionDiff> commonFunctionsDiffs = matcher.getCommonFunctions();
			
			// Setting up visualization module, if enabled
			GraphvizGenerator vis = null;
			if(visualizationOn){
				if(systemOuts) System.out.println("Visualization is on: setting up visualizer module...");
				vis = new GraphvizGenerator(GraphvizGenerator.DEFAULT_DIR
						+ "/" + (oldPath.getNameCount() >= 3 ?
								(oldPath.getName(oldPath.getNameCount() - 3).toString().startsWith("CVE") ?
										oldPath.getName(oldPath.getNameCount() - 3) : "")
								: "")
						+ "/FILE_OLD_" + 
						oldPath.getFileName().toString().substring(0, oldPath.getFileName().toString().indexOf('.')) + "_NEW_" +
						newPath.getFileName().toString().substring(0, newPath.getFileName().toString().indexOf('.')), FILE_EXTENSION.PDF);
			}
			
			//For every matched couple of functions...
			int modifiedFunctionsCounter = 0;
			for (FunctionDiff f: commonFunctionsDiffs) {
				
				if(logger.isInfoEnabled()) logger.info("FUNCTION - Old: {} : New: {}", f.getOriginalFunction(), f.getNewFunction());
				
				if(coarseGrainedDiffEnabled){
					f.setOldFile(oldPreprocessedFile);
					f.setNewFile(newPreprocessedFile);
					//Function coarse-grained diff: mapping the text diff chunks on the AST nodes
					//This only results in ASTDeltas: and ASTDelta contains a diff chunk
					//and the list of affected statements, without fine-grained information

					//COARSE-GRAINED DIFF
					f.generateCoarseGrainedDiff(diff);
				}
				
				//Function fine-grained diff: finding the AST diff at a node granularity
				f.generateFineGrainedDiff();
				
				if(containsSomeChanges(f)){
					modifiedFunctionsCounter++;
					if(systemOuts) System.out.println("> FUNCTION:");
					printCommonFunction(f.getOriginalFunction(), f.getNewFunction());
					if(systemOuts) System.out.println();
					
					//If the two functions are, in some way, different, we generate
					//graphs and read-write information
					if(systemOuts) System.out.println("> Generating function graphs...");
					f.generateGraphs();
					f.generateReadWriteInformation();
				}
				
				JoernGraphs oJg = f.getOriginalGraphs();
				JoernGraphs nJg = f.getNewGraphs();
				if (oJg == null || nJg == null) {
					// if(systemOuts) System.out.println("Graphs not generated for function: " + f.getOriginalFunction().name.getEscapedCodeStr());
					continue;
				}
				
				List<CFGNode> oldCFGNodes = oJg.getCfg().getVertices();
				List<CFGNode> newCFGNodes = nJg.getCfg().getVertices();
				
				List<ASTNode> oldS = new ArrayList<ASTNode>();
				List<ASTNode> newS = new ArrayList<ASTNode>();
				
				for(CFGNode oc:oldCFGNodes) {
					if(oc instanceof ASTNodeContainer) {
						ASTNodeContainer currC = (ASTNodeContainer)oc;
						oldS.add(currC.getASTNode());						
					}
				}
				
				for(CFGNode oc:newCFGNodes) {
					if(oc instanceof ASTNodeContainer) {
						ASTNodeContainer currC = (ASTNodeContainer)oc;
						newS.add(currC.getASTNode());
					}
				}
				
				GumtreeASTMapper mappy = new GumtreeASTMapper();
				//List<StatementMap> currStMap = mappy.getStatmentMap(oldS, newS);
				
				GumtreeASTMapper mappy1 = new GumtreeASTMapper();
				List<StatementMap> currStMap1 = mappy1.getStatmentMap(f.getOriginalFunction(), f.getNewFunction(), oldS, newS);
				
				if(containsFineGrainedChanges(f)){
					
					//.dot visualization of the function fine-grained diff
					if(visualizationOn){
						vis.functionDiffToDot(f);
						ASTCFGMapping m = new ASTCFGMapping(f);
						m.map();
						vis.cfgToDot(f.getOriginalGraphs().getCfg(), f.getOriginalFunction(), f.getOriginalGraphs().getDefUseCfg(), m, "ORIGINAL");
						vis.cfgToDot(f.getNewGraphs().getCfg(), f.getNewFunction(), f.getNewGraphs().getDefUseCfg(), m, "NEW");
					}
				} else {
					if(logger.isInfoEnabled()) logger.info("No fine-grained differences in function." + f.getOriginalFunction().name.getEscapedCodeStr()  + "\n");
				}
				
				if(coarseGrainedDiffEnabled && printCoarseGrainedDiff){
					if(!containsCoarseGrainedChanges(f)){
						if(logger.isInfoEnabled()) logger.info("No ASTDeltas in function." + f.getOriginalFunction().name.getEscapedCodeStr()  + "\n");
					} else {

						//For every coarse-grained change information (ASTDelta)...
						List<ASTDelta> deltas = f.getAllDeltas();

						/*
						 * Printing coarse-grained diff information
						 */
						if(systemOuts) System.out.println(">> COARSE GRAINED TEXT DIFF");
						for (ASTDelta d : deltas){

							if(systemOuts) System.out.println();
							if(systemOuts) System.out.println(">>> DIFF DELTA " + d);
							if(systemOuts) System.out.println(">>> TYPE: " + d.getType());
							
							d.generateFineGrainedDiff();
							
							//.dot visualization of the Delta
							if(visualizationOn)
								vis.deltaToDot(d, f);

							switch (d.getType()) {
							case INSERT:
								if(systemOuts) System.out.println("INSERTION:");
								if(systemOuts) System.out.println("\n" + FileDiff.linesToString(d.getNewChunk().getLines()));
								printAffectedStatements(d.getNewStatements());										
								break;

							case DELETE:
								if(systemOuts) System.out.println("DELETE:");
								if(systemOuts) System.out.println("\n" + FileDiff.linesToString(d.getOriginalChunk().getLines()));
								printAffectedStatements(d.getOriginalStatements());							
								break;

							case CHANGE:


								if(systemOuts) System.out.println("CHANGE---:");
								if(systemOuts) System.out.println("\n" + FileDiff.linesToString(d.getOriginalChunk().getLines()));
								printAffectedStatements(d.getOriginalStatements());

								if(systemOuts) System.out.println("\nCHANGE+++:\n");
								if(systemOuts) System.out.println("\n" + FileDiff.linesToString(d.getNewChunk().getLines()));
								printAffectedStatements(d.getNewStatements());						
								break;

							default:
								break;

							}

							// Delta read-write analysis
							if(systemOuts) System.out.println();
							if(systemOuts) System.out.println(">>> DELTA READ-WRITE ANALYSYS");
							if(systemOuts) System.out.println();
							printReadWriteAnalysis(d, f);

						}
					}
				}
				
				//Here we are still back to the function level
				if(containsFineGrainedChanges(f)){
					// Applying heuristics
					
//					boolean h1 = HeuristicsHelper.anyConditionImplication(f.getFineGrainedDiff(), types, false, false);
//					if(systemOuts) System.out.println(">> Any condition implication: " + h1);
//					if(systemOuts) System.out.println();
					
					HeuristicsDispatcher d = new HeuristicsDispatcher();
					d.setExclusiveChanges(true);
					//TODO try to enable it in the experiments
					d.setConsiderMovedConditions(false);
					d.enableHeuristic(HEURISTIC.MODIFIED_CONDITIONS);
					d.enableHeuristic(HEURISTIC.INSERTED_IFS);
					d.enableHeuristic(HEURISTIC.INIT_OR_ERROR);
					d.enableHeuristic(HEURISTIC.KNOWN_PATCHES);
					//d.enableHeuristic(HEURISTIC.CONDITIONS);
					d.enableHeuristic(HEURISTIC.FLOW_RESTRICTIVE);

					//TODO try to enable it and put an external timeout or put a timeout in Z3
					d.setBVEnabled(cmd.hasOption(SafepatchCmdInterface.OPTION.ENABLE_BV_OPTION));
					if(coarseGrainedDiffEnabled)
						d.setCoarseGrainedDiff(f.getAllDeltas());
					
					if(systemOuts) System.out.println("> Applying heuristics...");
					boolean security = d.isSecurityPatch(f);
					if(systemOuts) System.out.println("> Security-related: " + security);
					if(systemOuts) System.out.println("> " + d.getHeuristicResults());
					if(systemOuts) System.out.println("> " + d.getPatchFeatures());
					results.add(new FunctionResult(f.getOriginalFunction().name.getEscapedCodeStr(),
							d.getHeuristicResults(),
							d.getPatchFeatures(),
							security,
							f.getOriginalFunction().getLocation().startLine,
							f.getNewFunction().getLocation().startLine));
					
//					if(systemOuts) System.out.println("> Security-related: " + d.isSecurityPatch(f));
//					if(systemOuts) System.out.println("> " + d.getResult());
					
					//Testing the CFG implication approach
//					Z3DoubleCFGHandler diffToZ3 = new Z3DoubleCFGHandler();
//					DoubleCFGWalker cfgWalker = new DoubleCFGWalker(
//							f.getOriginalGraphs().getCfg(),
//							f.getNewGraphs().getCfg(),
//							new CFGASTDiffProvider(f.getFineGrainedDiff()),
//							diffToZ3);
//					cfgWalker.traverse();
					
				}
				
				if(containsSomeChanges(f)){
					if(systemOuts) System.out.println("> END_FUNCTION.");
				}
			}
			
			if(systemOuts) System.out.println("AFFECTED_FUNCTIONS=" + modifiedFunctionsCounter);
			if(modifiedFunctionsCounter == 0)
				if(systemOuts) System.out.println(
						"No affected functions parsed. The patch was probably part of what the preprocessing phase removed.\n");
			
			/*
			 * TODO:
			 * Solve problem: if the for some reason the thing crashes the pre-processed files are not deleted.
			 */
			Files.delete(Paths.get(oldPreprocessed));
			Files.delete(Paths.get(newPreprocessed));
			if(systemOuts) System.out.println("Done.");
			if(systemOuts) System.out.println();
			
			ObjectMapper mapper = new ObjectMapper();
			String jsonInString = mapper.writeValueAsString(results);
			System.out.println(jsonInString);

			if (output != null) {
				mapper.writeValue(new File(output), results);
			}
		
		} catch (Exception e) {
			e.printStackTrace();
		}

	}

	private static void checkCmdOptions() {
		if(cmd.hasOption(SafepatchCmdInterface.OPTION.VERBOSE)){
			systemOuts = true;
		}
		
		if(cmd.hasOption(SafepatchCmdInterface.OPTION.COARSE_GRAINED_DIFF)){
			coarseGrainedDiffEnabled = true;
			printCoarseGrainedDiff = true;
		}
		
		if(cmd.hasOption(SafepatchCmdInterface.OPTION.ENABLE_VISUALIZATION)){
			visualizationOn = true;
		}

		if (cmd.hasOption(SafepatchCmdInterface.OPTION.OUTPUT_PATH)) {
			output = cmd.getOptionValue(SafepatchCmdInterface.OPTION.OUTPUT_PATH);
		}
		
		for(OPTION o : SafepatchCmdInterface.OPTION.values())
			if(cmd.hasOption(o)) if(systemOuts) System.out.println("Specified option " + o);
	}

	private static void parseCommandLine(String[] args) {
		try {
			cmd.parseCommandLine(args);
		} catch (RuntimeException | ParseException ex) {
			printHelpAndTerminate(ex);
		}
	}

	private static void printHelpAndTerminate(Exception ex) {
		System.err.println(ex.getMessage());
		cmd.printHelp();
		System.exit(1);
	}

	public static void printCommonFunction(FunctionDef oldFunc, FunctionDef newFunc){
		if(systemOuts) System.out.println(oldFunc.name.getEscapedCodeStr() + 
				" - Oldfile: at line " + oldFunc.getLocation().startLine + 
				" - Newfile: at line " + newFunc.getLocation().startLine);
	}
	
	public static boolean containsDuplicates(List<FunctionDef> l){
		List<String> names = new ArrayList<String>();
		l.forEach(def -> names.add(def.name.getEscapedCodeStr()));

		return (new HashSet<String>(names).size() != names.size() ?
				true :
					false);
	}
	
	/*
	 * Returns true if function count in the two lists
	 * is the same but there is at least one element present in the
	 * old list that is not present in the new. 
	 */
	public static boolean functionRenamed(
			ArrayList<FunctionDef> oldFunctionList,
			ArrayList<FunctionDef> newFunctionList) {
		List<String> oldNames = new ArrayList<String>();
		oldFunctionList.forEach(f -> oldNames.add(f.name.getEscapedCodeStr()));
		List<String> newNames = new ArrayList<String>();
		newFunctionList.forEach(f -> newNames.add(f.name.getEscapedCodeStr()));
		if(newNames.size() == oldNames.size()){
			newNames.removeAll(oldNames);
			if(newNames.size() != 0) return true;
		}
			
		return false;
	}
	
	public static boolean containsFineGrainedChanges(FunctionDiff f){
		return f.getFineGrainedDiff() != null && f.getFineGrainedDiff().getGumtreeActions().size() != 0; 
	}
	
	public static boolean containsCoarseGrainedChanges(FunctionDiff f){
		return f.getAllDeltas() != null && f.getAllDeltas().size() != 0;
	}
	
	public static boolean containsSomeChanges(FunctionDiff f){
		return containsCoarseGrainedChanges(f) || containsFineGrainedChanges(f);
	}
	
	/* 	----------------------
    		PRINTING METHODS
  		----------------------	*/
	static void printAffectedStatements(List<Statement> statements){
		if(logger.isInfoEnabled()) logger.info(">>>> Affected statements:");
		for (Statement stmt : statements){
			if(logger.isInfoEnabled()) logger.info(stmt);
			//Generation of the corresponding code to check that it matches the code of the diff chunk.
			//if(logger.isDebugEnabled()) logger.debug("Generating code:\n" + JoernUtil.genCode(stmt));
		}
	}
	
	static void printReadWriteAnalysis(ASTDelta d, FunctionDiff f){

		boolean isRead = (d == null ? f.getReadWriteDiff().isRead() :
			f.getReadWriteDiff().isRead(d));
		if(systemOuts) System.out.println("> IsRead: " + isRead);
		if(systemOuts) System.out.println();
		List<UseOrDefSymbol> l;
		if(isRead){
			if(systemOuts) System.out.println(">> Inserted reads: ");
			l = (d == null ? f.getReadWriteDiff().insertedReads() :
				f.getReadWriteDiff().insertedReads(d));
			printReadWriteAnalysisHelper(l);

			if(systemOuts) System.out.println(">> Deleted reads: ");
			l = (d == null ? f.getReadWriteDiff().deletedReads() :
				f.getReadWriteDiff().deletedReads(d));
			printReadWriteAnalysisHelper(l);
		}

		boolean isWrite = (d == null ? f.getReadWriteDiff().isWrite() :
			f.getReadWriteDiff().isWrite(d));
		if(systemOuts) System.out.println("> IsWrite: " + isWrite);
		if(systemOuts) System.out.println();
		
		if(isWrite){
			if(systemOuts) System.out.println(">> Inserted writes: ");
			l = (d == null ? f.getReadWriteDiff().insertedWrites() :
				f.getReadWriteDiff().insertedWrites(d));
			printReadWriteAnalysisHelper(l);

			if(systemOuts) System.out.println(">> Deleted writes: ");
			l = (d == null ? f.getReadWriteDiff().deletedWrites() :
				f.getReadWriteDiff().deletedWrites(d));
			printReadWriteAnalysisHelper(l);
		}
		
	}
	
	static void printReadWriteAnalysisHelper(List<UseOrDefSymbol> l){
		if(l.size() != 0){
		if(systemOuts) System.out.println(l);
		if(systemOuts) System.out.println(ReadWriteTable.useOrDefSymbolsToIdentifierSet(l));
		if(systemOuts) System.out.println(ReadWriteTable.useOrDefSymbolsToValueSet(l));
		} else {
			if(systemOuts) System.out.println("None");
		}
		if(systemOuts) System.out.println();
	}
	
//	static void printAllGraphs(FunctionDiff f){
//		/*
//		 * Function used to check that the graphs are correct.
//		 */
//		JoernGraphs oldGraphs = f.getOriginalGraphs();
//		JoernGraphs newGraphs = f.getNewGraphs();
//
//		if(systemOuts) System.out.println("\nOLD CFG:");
//		if(systemOuts) System.out.println(oldGraphs.getCfg());
//		if(systemOuts) System.out.println("\nNEW CFG:");
//		if(systemOuts) System.out.println(newGraphs.getCfg());
//		
//		if(systemOuts) System.out.println("\nOLD CFG VERTICES WITH AST NODES:");
//		JoernUtil.printCfgWithASTNodes(oldGraphs.getCfg());
//		if(systemOuts) System.out.println("\nNEW CFG VERTICES WITH AST NODES:");
//		JoernUtil.printCfgWithASTNodes(newGraphs.getCfg());
//		
//		if(systemOuts) System.out.println("\nOLD CDG:");
//		JoernUtil.printCdg(oldGraphs.getCdg());
//		if(systemOuts) System.out.println("\nDOMINATOR TREE:");
//		JoernUtil.printDominatorTree(oldGraphs.getCdg().getDominatorTree());
//		if(systemOuts) System.out.println("\nNEW CDG:");
//		JoernUtil.printCdg(newGraphs.getCdg());
//		if(systemOuts) System.out.println("\nDOMINATOR TREE:");
//		JoernUtil.printDominatorTree(newGraphs.getCdg().getDominatorTree());
//		
//		
//		if(systemOuts) System.out.println("\nOLD UDG:");
//		JoernUtil.printUDG(oldGraphs.getUdg());
//		if(systemOuts) System.out.println("\nNEW UDG:");
//		JoernUtil.printUDG(newGraphs.getUdg());
//		
//		if(systemOuts) System.out.println("\nOLD DEF-USE CFG:");
//		JoernUtil.printDefUseCFG(oldGraphs.getDefUseCfg());
//		if(systemOuts) System.out.println("\nNEW DEF-USE CFG:");
//		JoernUtil.printDefUseCFG(newGraphs.getDefUseCfg());
//		
//		if(systemOuts) System.out.println("\nOLD DDG:");
//		JoernUtil.printDdg(oldGraphs.getDdg());
//		if(systemOuts) System.out.println("\nNEW DDG:");
//		JoernUtil.printDdg(newGraphs.getDdg());
//		
//		if(systemOuts) System.out.println("\nDDG DIFF:");
//		f.getDdgDiff();
//	}
//	
//	static void printUseDefAnalysis(ASTDelta d, FunctionDiff f){
//		if(systemOuts) System.out.println();
//		//if(systemOuts) System.out.println(">>>> isWrite: " + ReadWriteUtil.isWrite(d, f));
//		Map<ASTNode, List<Object>> out = DefUseUtil.insertedDefs(d, f);
//		if(systemOuts) System.out.println("Inserted writes: " + out);
//		if(systemOuts) System.out.println("  In nodes: ");
//		for(ASTNode n : out.keySet())
//			JoernUtil.printASTNode(n, 2);
//		
//		out = DefUseUtil.deletedDefs(d, f);
//		if(systemOuts) System.out.println("Deleted writes: " + out);
//		if(systemOuts) System.out.println("  In nodes: ");
//		for(ASTNode n : out.keySet())
//			JoernUtil.printASTNode(n, 2);
//		
//		if(systemOuts) System.out.println();
//		//if(systemOuts) System.out.println(">>>> isRead: " + ReadWriteUtil.isRead(d, f));
//		
//		out = DefUseUtil.insertedUses(d, f);
//		if(systemOuts) System.out.println("Inserted reads: " + out);
//		if(systemOuts) System.out.println("  In nodes: ");
//		for(ASTNode n : out.keySet())
//			JoernUtil.printASTNode(n, 2);
//		
//		out = DefUseUtil.deletedUses(d, f);
//		if(systemOuts) System.out.println("Deleted reads: " + DefUseUtil.deletedUses(d, f));
//		if(systemOuts) System.out.println("  In nodes: ");
//		for(ASTNode n : out.keySet())
//			JoernUtil.printASTNode(n, 2);
//	}
}

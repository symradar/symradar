/**
 * 
 */
package tools.safepatch.util;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;

import org.apache.commons.cli.ParseException;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.fasterxml.jackson.databind.ObjectMapper;

import ast.functionDef.FunctionDef;
import ast.statements.Statement;
import fileWalker.FileNameMatcher;
import parsing.ModuleParser;
import parsing.C.Modules.ANTLRCModuleParserDriver;
import parsing.C.Modules.CModuleParserTreeListener;
import tools.CommonCommandLineInterface;
import tools.safepatch.FunctionMatcher;
import tools.safepatch.FunctionMatcher.MATCHING_METHOD;
import tools.safepatch.HeuristicsDispatcher;
import tools.safepatch.HeuristicsDispatcher.HEURISTIC;
import tools.safepatch.HeuristicsDispatcher.PATCH_FEATURE;
import tools.safepatch.SafepatchASTWalker;
import tools.safepatch.SafepatchCmdInterface;
import tools.safepatch.SafepatchCmdInterface.OPTION;
import tools.safepatch.SafepatchPreprocessor;
import tools.safepatch.SafepatchPreprocessor.PREPROCESSING_METHOD;
import tools.safepatch.cfgimpl.ASTCFGMapping;
import tools.safepatch.cgdiff.FileDiff;
import tools.safepatch.diff.ASTDelta;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.rw.ReadWriteTable;
import tools.safepatch.visualization.GraphvizGenerator;
import tools.safepatch.visualization.GraphvizGenerator.FILE_EXTENSION;
import udg.symbols.UseOrDefSymbol;

/**
 * @author machiry
 * @author Eric Camellini
 *
 */
public class PrototypesExtractor {

	
	private static class OneFileCmdInterface extends CommonCommandLineInterface {
		
		public String filename;
		public void parseCommandLine(String[] args) throws ParseException
		{
			if (args.length != 1)
				throw new RuntimeException(
						"only 1 argument needed.");

			cmd = parser.parse(options, args);
			this.filename = cmd.getArgs()[0];
			
		}

		public void printHelp()
		{
			formater.printHelp("One argument needed: a .c file.", options);
		}

		public String getFilename() {
			return this.filename;
		}
	}
	
	final static String C_FILENAME_FILTER = "*.c";
	
	static FileNameMatcher matcher = new FileNameMatcher();

	static ANTLRCModuleParserDriver driver = new ANTLRCModuleParserDriver();
	static ModuleParser parser = new ModuleParser(driver);
	static SafepatchASTWalker astWalker = new SafepatchASTWalker();
	
	
	public static void main(String[] args)
	{
		
		// Reading arguments
		OneFileCmdInterface cmd = new OneFileCmdInterface();
		parseCommandLine(cmd, args);
		String fileName = cmd.getFilename();
		
		matcher.setFilenameFilter(C_FILENAME_FILTER);
		parser.addObserver(astWalker);
		
		
		try {
			parser.parseFile(fileName);
			
			//After parsing a file the SafepatchASTWalker saves a list of the function AST nodes in a Map,
			//for every parsed file.
			ArrayList<FunctionDef> oldFunctionList = astWalker.getFileFunctionList().get(fileName);
			oldFunctionList.forEach(f -> {System.out.println(f.getChild(1).getEscapedCodeStr() + " " + f.getFunctionSignature());});
		
		} catch (Exception e) {
			e.printStackTrace();
		}

	}

	private static void parseCommandLine(OneFileCmdInterface cmd, String[] args) {
		try {
			cmd.parseCommandLine(args);
		} catch (RuntimeException | ParseException ex) {
			printHelpAndTerminate(cmd, ex);
		}
	}
	
	private static void printHelpAndTerminate(OneFileCmdInterface cmd, Exception ex) {
		System.err.println(ex.getMessage());
		cmd.printHelp();
		System.exit(1);
	}
}

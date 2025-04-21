package tools.safepatch.util;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import java.util.Set;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.util.HashSet;

import ast.ASTNode;
import ast.functionDef.FunctionDef;
import ast.statements.Statement;
import ast.walking.ASTNodeVisitor;
import cfg.ASTToCFGConverter;
import cfg.CFG;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;
import ddg.CFGAndUDGToDefUseCFG;
import ddg.DefUseCFG.DefUseCFG;
import parsing.ModuleParser;
import parsing.C.Modules.ANTLRCModuleParserDriver;
import tools.safepatch.SafepatchASTWalker;
import tools.safepatch.SafepatchMain;
import tools.safepatch.SafepatchPreprocessor;
import tools.safepatch.SafepatchPreprocessor.PREPROCESSING_METHOD;
import tools.safepatch.rw.ReadWriteTable;
import tools.safepatch.visualization.GraphvizGenerator;
import udg.CFGToUDGConverter;
import udg.useDefGraph.UseDefGraph;

public class AffectedVariablesUtil {
	
	private static final Logger logger = LogManager.getLogger();
	
	/* 
	 * Assumptions:
	 *   - if a is modified, to be sound, we considered also *a and &a as modified
	*/
	private static Collection<String> getClosureSym(Collection<String> old_syms) {
		Set<String> new_syms = new HashSet<String>(old_syms);
		
		for (String s : (Set<String>)old_syms) {
		    if (!s.startsWith("&") && !s.startsWith("*")){
		    	new_syms.add("* " + s);
		    	new_syms.add("& " + s);
		    }
		}
		
		return (Collection<String>) new_syms;
	}
	
	/* 
	 * Returns the list of variables affected by a given set of variables
	 * checking recursively in the code.
	 */
	public static Set<String> getAffectedVars(Set<String> init_sym, CFG cfg, DefUseCFG ducfg ){
		boolean fixpoint = false;
		Set<String> closure_sym = new HashSet<String>(init_sym);
		if(logger.isDebugEnabled()) logger.debug("Finding affected var closure starting from {}", closure_sym);
		while(!fixpoint) {
			closure_sym = (Set<String>) getClosureSym((Collection<String>) closure_sym);
			fixpoint = true;
			for (CFGNode v : (List<CFGNode>)cfg.getVertices()){
				if (! (v instanceof ASTNodeContainer))
					continue;
				
				LinkedList<Object> use = (LinkedList<Object>)ducfg.getSymbolsUsedBy(((ASTNodeContainer) v).getASTNode());
				ListIterator<Object> nodeIterator = use.listIterator();
				while (nodeIterator.hasNext()) {
					String n_var = (String) nodeIterator.next();
					if (closure_sym.contains(n_var)) {
						LinkedList <Object> defs = (LinkedList<Object>)ducfg.getSymbolsDefinedBy(((ASTNodeContainer) v).getASTNode());
						ListIterator<Object> defIterator = defs.listIterator();
						while (defIterator.hasNext()){
							String s = (String) defIterator.next();
							if (closure_sym.add(s)){
								if(logger.isDebugEnabled()) logger.debug("Added {} because of {}", s , v);
								fixpoint = false;
							}
						}
					}
				}					
			}					
		}
		return closure_sym;
	}
	
	/**
	 * Returns the variables affected by the statements.
	 */
	public static Set<String> getAffectedVars(Collection<ASTNode> statements, DefUseCFG ducfg, boolean ignoreCConstants){
		if(logger.isDebugEnabled()) logger.debug("Finding vars affected by statements {}", statements);
		Set<String> affectedVars = new HashSet<String>();
		for(ASTNode s : statements){
			for(Object var : ducfg.getSymbolsDefinedBy(s)){
				if(ignoreCConstants && ((String) var).toUpperCase().equals((String)var)) continue;
				if(affectedVars.add((String) var)) 
					if(logger.isDebugEnabled()) logger.debug("Added {} because of {}", var , s);
			}
		}
		return affectedVars;
	}
	
	static ANTLRCModuleParserDriver driver = new ANTLRCModuleParserDriver();
	static ModuleParser parser = new ModuleParser(driver);
	static SafepatchASTWalker astWalker = new SafepatchASTWalker();
	
	/* Z3 renamer tester */
	public static void main(String[] args) {
		parser.addObserver(astWalker);
		
		String func_name = "main";
		String filename = "../test_files/ddg.c"; // FIXME: take input
		Path Path = Paths.get(filename);
		String Preprocessed = Path.toString().replaceAll(Path.getFileName().toString(), 
				SafepatchMain.PREPROCESSED_FILE_PREFIX + Path.getFileName());

		// FIXME: this should be passed from the user
		Set<String> changed_sym = new HashSet<String>();
		changed_sym.add("b");
		
		// END FIXME
		try {
			SafepatchPreprocessor prep = new SafepatchPreprocessor(PREPROCESSING_METHOD.UNIFDEFALL);
			prep.preProcess(filename, Preprocessed);
			parser.parseFile(Preprocessed);
			
			
			ArrayList<FunctionDef> FunctionList = astWalker.getFileFunctionList().get(Preprocessed);
			
			for (FunctionDef f : FunctionList)
			{	
				if(!f.name.getEscapedCodeStr().equals(func_name)){
					continue;
				}
				
				// We have to do all this mess to get the use def cfg because we do not
				// create the DB
				ASTToCFGConverter conv = new ASTToCFGConverter();
				CFG cfg = conv.convert(f);
				CFGToUDGConverter udgConv = new CFGToUDGConverter();
				UseDefGraph udg = udgConv.convert(cfg);
				CFGAndUDGToDefUseCFG useDefCfgCreator = new CFGAndUDGToDefUseCFG();
				DefUseCFG ducfg = useDefCfgCreator.convert(cfg, udg);
				GraphvizGenerator gv = new GraphvizGenerator();
				gv.cfgToDot(cfg, f, ducfg);
				Collection<String> superset = getAffectedVars(changed_sym, cfg, ducfg);

				System.out.println(Arrays.toString(superset.toArray()));
			}
	
				Files.delete(Paths.get(Preprocessed));
			} catch (Exception e) {
				e.printStackTrace();
			}


	}

}


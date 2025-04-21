package tools.safepatch.visualization;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.StringWriter;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.apache.commons.io.FileUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.github.gumtreediff.tree.ITree;
import com.google.common.collect.Sets.SetView;
import com.sun.nio.sctp.AssociationChangeNotification;

import ast.ASTNode;
import ast.functionDef.FunctionDef;
import ast.statements.Statement;
import cfg.CFG;
import cfg.CFGEdge;
import cfg.nodes.ASTNodeContainer;
import cfg.nodes.CFGNode;
import databaseNodes.NodeKeys;
import ddg.DefUseCFG.DefUseCFG;
import tools.safepatch.cfgimpl.CFGMapping;
import tools.safepatch.diff.ASTDelta;
import tools.safepatch.diff.FunctionDiff;
import tools.safepatch.diff.ASTDelta.DELTA_TYPE;
import tools.safepatch.fgdiff.ASTDiff;
import tools.safepatch.rw.ReadWriteTable;
import tools.safepatch.util.JoernUtil;
import udg.useDefGraph.UseDefGraph;


/**
 * @author Eric Camellini
 *
 */
public class GraphvizGenerator {

	/* EXAMPLE OF DOT
	 * 
	 * 	//https://en.wikipedia.org/wiki/DOT_(graph_description_language)
		//http://graphs.grevian.org/documentation
		
		graph {
		    vr[label="Red vertex", color="red"];
		    vg[label="Green vertex", color="green"];
		    vb[label="Blue vertex", color="blue"];
		    vy[label="Yellow vertex", color="orange"];
		    vn[label="Normal vertex"];
		    vnl[label="Vertex with a super long name"];
		
		    vnl -- vn;
		    vn -- vr;
		    vnl -- vg;
		    vg -- vy;
		    vg -- vb;
		}
	 * 
	 */
	
	/*
	 * TODO:
	 *  - Refactor the whole class, it's super badly coded
	 *  - Add more file formats
	 *  - Find a way to render in a better way big graphs
	 */
	
	public enum FILE_EXTENSION{
		PDF("pdf"), PNG("png"), BMP("bmp"), SVG("svg"), DOT("dot"),
		GIF("gif"), JPEG("jpeg"), PLAIN("plain"), TIFF("tiff"), JSON("json");
		
		private String extension;
		
		private FILE_EXTENSION(String extension) {
			this.extension = extension;
		}
		public String getExtension() {
			return extension;
		}
		
	}

	private static final Logger logger = LogManager.getLogger();
	
	private Map<Object, String> dotNodes;
	private ArrayList<String> edges;
	private ArrayList<String> graphs;
	
	String outputDirectory;
	FILE_EXTENSION ext = FILE_EXTENSION.PDF; //Default
	
	public static final String DEFAULT_DIR = "output_graphs";
	
	private static final String ORIGINAL = "ORIGINAL";
	private static final String NEW = "NEW";
	
	// UNFLATTEN PARAMETERS
	/*
	 * TODO
	 * SOLVE THIS PROBLEM:
	 *  - Unflattening results in a better output but the node order changes.
	 */
	private boolean unflatten = false;
	private int lparam = 10;
	
	public void setLparam(int lPARAM) {
		lparam = lPARAM;
	}

	public void setUnflatten(boolean unflatten) {
		this.unflatten = unflatten;
	}

	public GraphvizGenerator() {
		this.init(DEFAULT_DIR);
	}
	
	public GraphvizGenerator(String outputDir) {
		this.init(outputDir);
	}
	
	public GraphvizGenerator(String outputDir, FILE_EXTENSION ext) {
		this.ext = ext;
		this.init(outputDir);
	}
	
	private void init(String outputDir){
		this.dotNodes = new HashMap<Object, String>();
		this.edges = new ArrayList<>();
		this.graphs = new ArrayList<>();
		this.outputDirectory = outputDir;
		
		try {
			FileUtils.forceDelete(new File(this.outputDirectory));
		} catch (IOException e1) {
			System.err.println("WARN: Trying to clean a non existing directory.");
		}
		
		try {
			Files.createDirectories(Paths.get(this.outputDirectory));
		} catch (IOException e) {
			System.err.println("WARN: Output directory already exixts or not created.");
		}
		
	}
	
	public void FunctionDefToDot(FunctionDef f) throws IOException, InterruptedException{
		this.clear();
		this.astNodeToDot(f, null, null);
		String file = this.outputDirectory + "/" + "FUNCTION_" + Integer.toHexString(f.hashCode()) +  "_" + f.name.getEscapedCodeStr();
		this.flushGraph("FunctionGraph");
		this.writeDotFile(file);
		this.generateGraph(file  + ".dot");
	}
	
	public void functionDiffToDot(FunctionDiff f) throws IOException, InterruptedException{
		this.clear();
		this.astNodeToDot(f.getOriginalFunction(), f.getFineGrainedDiff(), f.getOriginalReadWriteTable());
		String file = this.outputDirectory + "/" + "FUNCTION_" + Integer.toHexString(f.hashCode()) + "_" + ORIGINAL + "_" + f.getOriginalFunction().name.getEscapedCodeStr();
		this.flushGraph(ORIGINAL);
		this.writeDotFile(file);
		this.generateGraph(file  + ".dot");
		
		this.clear();
		this.astNodeToDot(f.getNewFunction(), f.getFineGrainedDiff(), f.getNewReadWriteTable());
		file = this.outputDirectory + "/" + "FUNCTION_" + Integer.toHexString(f.hashCode()) + "_" + NEW + "_" + f.getNewFunction().name.getEscapedCodeStr();
		this.flushGraph(NEW);
		this.writeDotFile(file);
		this.generateGraph(file  + ".dot");
		
		if(f.getAllDeltas() != null && f.getAllDeltas().size() != 0){
			String fdir = this.outputDirectory + "/" + functionDeltasDirName(f);
			try {
				Files.createDirectory(Paths.get(fdir));
			} catch (IOException e) {
				if(logger.isWarnEnabled()) logger.warn("Function directory already exixts or not created.");
			}
			
			try {
				FileUtils.cleanDirectory(new File(fdir));
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}
	
	private String functionDeltasDirName(FunctionDiff f){
		return "FUNCTION_" + Integer.toHexString(f.hashCode()) + "_DELTAS";
	}
	
	public void deltaToDot(ASTDelta d, FunctionDiff f) throws IOException, InterruptedException{
		if(!f.getAllDeltas().contains(d))
			throw new RuntimeException("The ASTDelta d must be a delta contained in the function f");
		
		String file = "";
		if(d.getType() == DELTA_TYPE.DELETE || d.getType() == DELTA_TYPE.CHANGE){
			this.clear();
			int i = 0;
			for(Statement s : d.getOriginalStatements()){
				this.astNodeToDot(s, d.getFineGrainedDiff(), f.getOriginalReadWriteTable());
				this.flushGraph("\"" + ORIGINAL + Integer.toHexString(s.hashCode()) + "\"");
				file = this.outputDirectory + 
						"/" + functionDeltasDirName(f) +
						"/" + d.getType() + "_" + Integer.toHexString(d.hashCode()) + "_STMT" + Integer.toString(i) + (d.getType() == DELTA_TYPE.CHANGE ? "_" + ORIGINAL : "");
				this.writeDotFile(file);
				this.generateGraph(file  + ".dot");
				this.clear();
				i++;
			}			
		}
		
		if(d.getType() == DELTA_TYPE.INSERT || d.getType() == DELTA_TYPE.CHANGE){
			this.clear();
			int i = 0;
			for(Statement s : d.getNewStatements()){
				this.astNodeToDot(s, d.getFineGrainedDiff(), f.getNewReadWriteTable());
				this.flushGraph("\"" + NEW + Integer.toHexString(s.hashCode()) + "\"");
				file = this.outputDirectory + 
						"/" + functionDeltasDirName(f) +
						"/" + d.getType() + "_" + Integer.toHexString(d.hashCode()) + "_STMT" + Integer.toString(i) + (d.getType() == DELTA_TYPE.CHANGE ? "_" + NEW : "");
				this.writeDotFile(file);
				this.generateGraph(file  + ".dot");
				this.clear();
				i++;
			}
		}
	}

	public void astNodeToDot(ASTNode n, ASTDiff diff, ReadWriteTable r){
		addNodeToDotNodes(n, diff, r);
		
		for(int i = 0 ; i < n.getChildCount() ; i++){
			this.edges.add(getASTNodeDotId(n) + " -> " + getASTNodeDotId(n.getChild(i)) + ";");
			astNodeToDot(n.getChild(i), diff, r);
		}
	}
	
	private String astNodeToDotString(ASTNode n, ASTDiff diff, ReadWriteTable r){
		
		//ITree itree = diff.getMapper().getMappedITree(n);
		ASTNode mapped = null;

		if(diff != null){
			//if(diff.isUpdated(n) || diff.isMoved(n)){
			mapped = diff.getMappedNode(n);
			/*mapped = diff.getMapper().getMappedASTNode(
					diff.getMappings().getDst(itree) != null ? 
							diff.getMappings().getDst(itree) : 
								diff.getMappings().getSrc(itree));*/

		}
		
		Set<String> reads = null;
		Set<String> writes = null;
		if(r != null){
			reads = r.getReadSymbolValues(n);
			writes = r.getWriteSymbolValues(n);
		}
		
		String retval =  getASTNodeDotId(n) + "["
				+ "label=" + "\"" + n.getClass().getSimpleName() + "@" + Integer.toHexString(n.hashCode())
				+ (n.getLabel().length() != 0 ? System.getProperty("line.separator") + "AST label:   " + n.getLabel().replace("\\", "\\\\").replace("\"", "\\\"") : "")  
				+ (mapped != null ? System.getProperty("line.separator") + "Mapped to:   " + Integer.toHexString(mapped.hashCode()) : "")
				+ (reads != null ? System.getProperty("line.separator") + "Reads:   " + reads : "")
				+ (writes != null ? System.getProperty("line.separator") + "Writes:   " + writes : "")
				+ "\""
				+ (diff == null ? "" : 
					(diff.isDeleted(n) ? ", fillcolor=\"indianred\"" :
						(diff.isUpdated(n) ? ", fillcolor=\"gold\"" :
							(diff.isMoved(n) ? ", fillcolor=\"lightskyblue\"" :
								(diff.isInserted(n) ? ", fillcolor=\"springgreen\"" : "")))))
				+ "];";
		
		return retval;
	}
	
	private String getASTNodeDotId(ASTNode n){
		return Long.toString(n.getIDForGumTree());
	}
	
	private void addNodeToDotNodes(ASTNode n, ASTDiff diff, ReadWriteTable r){
		if(dotNodes.get(n) == null)
			dotNodes.put(n, astNodeToDotString(n, diff, r));
	}
	
	public void cfgToDot(CFG cfg, FunctionDef f, DefUseCFG ducfg) throws IOException, InterruptedException{
		cfgToDot(cfg, f, ducfg, null, null);
	}
	
	public void cfgToDot(CFG cfg, FunctionDef f, DefUseCFG ducfg, CFGMapping m, String nameSuffix) throws IOException, InterruptedException{
		this.clear();
		this.addCFGVertices(cfg.getVertices(), ducfg, m);
		this.addCFGEdges(cfg.getEdges());
		String file = this.outputDirectory + "/" + "FUNCTION_" + Integer.toHexString(f.hashCode()) + 
				(nameSuffix != null ? "_" + nameSuffix : "")
				+ "_" + f.name.getEscapedCodeStr() + "_CFG";
		this.flushGraph("FunctionCfg");
		this.writeDotFile(file);
		this.generateGraph(file  + ".dot");
	}
	
	private void addCFGEdges(List<CFGEdge> edges) {
		for(CFGEdge e : edges){
			this.edges.add(getCFGNodeDotId(e.getSource()) + " -> " + getCFGNodeDotId(e.getDestination()) 
			+ (e.getLabel() != null && e.getLabel().length() != 0 ? "[ label=\"" + e.getLabel() + "\"]" : "")
			+ ";");
		}
	}

	private void addCFGVertices(List<CFGNode> vertices, DefUseCFG ducfg, CFGMapping m) {
		for(CFGNode v : vertices){
			if(dotNodes.get(v) == null)
				dotNodes.put(v, cfgNodeToDotString(v, ducfg, m));
		}
	}

	private String cfgNodeToDotString(CFGNode v, DefUseCFG ducfg, CFGMapping m) {
		String code = (String) v.getProperties().get(NodeKeys.CODE);
		CFGNode mapped = null;
		if(m != null)
			mapped = m.getMappedNode(v);
		
		String retval =  getCFGNodeDotId(v) + "["
				+ "label=" + "\"" + Integer.toHexString(v.hashCode()) + System.getProperty("line.separator")
				+ code.replace("\\", "\\\\").replace("\"", "\\\"") +
				(ducfg != null && v instanceof ASTNodeContainer ?
				System.getProperty("line.separator")
				+ "Defs: " + ducfg.getSymbolsDefinedBy(((ASTNodeContainer) v).getASTNode()) 
				+ System.getProperty("line.separator")
				+ "Use: " + ducfg.getSymbolsUsedBy(((ASTNodeContainer) v).getASTNode()) : "") +
				(mapped != null ? 
						System.getProperty("line.separator") +
						"Mapped to :" +  Integer.toHexString(mapped.hashCode()) : "") +
				(v instanceof ASTNodeContainer ? System.getProperty("line.separator") 
						+ "Node: " + ((ASTNodeContainer) v).getASTNode().getClass().getSimpleName() : "") + 
				"\""
				+ "];";
		
		return retval;
	}

	private String getCFGNodeDotId(CFGNode v) {
		return Integer.toString(v.hashCode());
	}

	public void flushGraph(String name){
		String output = "digraph "+ name +" {" + System.getProperty("line.separator")
		+ "ordering=\"out\";" + System.getProperty("line.separator")
		//+ "size=\"6,6\";" + System.getProperty("line.separator")
		//+ "splines=\"curved\";" + System.getProperty("line.separator")
		+ "node [color=black, fillcolor=azure2, style=\"filled, rounded\", shape=box ];" 
		+ System.getProperty("line.separator")
		;
		for(Object n : dotNodes.keySet())
			output += dotNodes.get(n) + System.getProperty("line.separator");
		output += System.getProperty("line.separator");
		
		for(String e : this.edges)
			output += e + System.getProperty("line.separator");
		
		output += "}";
		this.graphs.add(output);
		this.clearGraph();
	}
	
	public String getDotFileContent(){
		String output = "";
		for(String graph : this.graphs)
			output += graph;
		return output;
	}
	
	public void writeDotFile(String filename) throws IOException{
		if(!filename.endsWith(".dot"))
			filename += ".dot";
		Files.write(Paths.get(filename), getDotFileContent().getBytes(StandardCharsets.UTF_8));
	}
	
	private void clear(){
		this.graphs.clear();
		this.clearGraph();
	}
	
	private void clearGraph(){
		this.dotNodes.clear();
		this.edges.clear();
	}
	
	public void generateGraphs(String directory) throws IOException{
		Files.list(Paths.get(directory))
		.forEach(f -> {
			try {
				dotCommand(f);
			} catch (IOException | InterruptedException e) {
				e.printStackTrace();
			}
		});
	}
	
	public void generateGraph(String dotfile) throws IOException, InterruptedException{
		if(!dotfile.endsWith(".dot"))
			throw new RuntimeException("Not a .dot file");
		
		dotCommand(Paths.get(dotfile));
	}
	
	private void dotCommand(Path f) throws IOException, InterruptedException{
		Process p;
		if(this.unflatten){
			Path unflattened = Paths.get(f.toString().replace(".dot", ".unflattened.dot"));
			String pre = "unflatten -f -l " + Integer.toString(lparam) + " " + f + " -o " + unflattened;
			if(logger.isDebugEnabled()) logger.debug("Executing: {}", pre);
			p = Runtime.getRuntime().exec(pre);
			p.waitFor();
			f = unflattened;
		}
		String command = "dot " + f + " -T" + ext.getExtension() + " -O";
		if(logger.isDebugEnabled()) logger.debug("Executing: {}", command);
		p = Runtime.getRuntime().exec(command);
		p.waitFor();
//
//		InputStream in = new BufferedInputStream( p.getInputStream());
//		OutputStream out = new BufferedOutputStream( new FileOutputStream( f.toString().replace(".dot", "." + ext.getExtension()) ));
//
//		int cnt;
//		byte[] buffer = new byte[1024];
//		while ( (cnt = in.read(buffer)) != -1) {
//		   out.write(buffer, 0, cnt );
//		}
//		
//		out.close();
	}
}

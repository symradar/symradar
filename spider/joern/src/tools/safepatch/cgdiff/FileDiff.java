package tools.safepatch.cgdiff;

import difflib.Delta;
import difflib.DiffUtils;
import difflib.Patch;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
/**
 * @author Eric Camellini
 * @see https://www.adictosaltrabajo.com/tutoriales/comparar-ficheros-java-diff-utils/
 * 
 */
public class FileDiff {
	
	private File original;
	private File revised;

	private List<Delta<String>> allDeltas;
	private List<Delta<String>> insertions;
	private List<Delta<String>> deletes;
	private List<Delta<String>> changes;



	public List<Delta<String>> getAllDeltas() {
		return allDeltas;
	}

	public List<Delta<String>> getInsertions() {
		return insertions;
	}


	public List<Delta<String>> getDeletes() {
		return deletes;
	}

	public List<Delta<String>> getChanges() {
		return changes;
	}

	public File getOriginal() {
		return original;
	}

	public File getRevised() {
		return revised;
	}

	public FileDiff(File original, File revised) {
		this.original = original;
		this.revised = revised;
	}


	public void diff() throws IOException{
		this.allDeltas = getDeltas();
		this.changes = new ArrayList<Delta<String>>();
		this.deletes = new ArrayList<Delta<String>>();
		this.insertions = new ArrayList<Delta<String>>();
		for (Delta<String> delta : this.allDeltas) {
			switch (delta.getType()) {
			case CHANGE:
				this.changes.add(delta);
				break;

			case INSERT:
				this.insertions.add(delta);
				break;

			case DELETE:
				this.deletes.add(delta);
				break;

			default:
				break;
			}
		}
	}

	public List<Delta<String>> getDeltasByType(Delta.TYPE type) {
		switch (type) {
		case CHANGE:
			return this.changes;

		case INSERT:
			return this.insertions;

		case DELETE:
			return this.deletes;

		default:
			return null;
		}
	}

	private List<Delta<String>> getDeltas() throws IOException {

		List<String> originalFileLines = fileToLines(original);
		List<String> revisedFileLines = fileToLines(revised);

		Patch<String> patch = DiffUtils.diff(originalFileLines, revisedFileLines);

		return patch.getDeltas();
	}

	private List<String> fileToLines(File file) throws IOException {
		List<String> lines = new ArrayList<String>();
		String line;
		BufferedReader in = new BufferedReader(new FileReader(file));
		while ((line = in.readLine()) != null) {
			lines.add(line);
		}
		in.close();
		return lines;
	}

	public static String linesToString(List<String> lines){
		String s = "";
		for (Object line : lines)
			s += line + "\n";
		return s;

	}


	//TEST MAIN
	public static void main(String[] args)
	{
		//String oldf = "../examples/android_security_bulletin/08-2016/CVE-2016-3850/old_latest/aboot.c";
		//String newf = "../examples/android_security_bulletin/08-2016/CVE-2016-3850/new/aboot.c";

		String oldf = "../test_files/test_old";
		String newf = "../test_files/test_new";
		File oldFile = new File(oldf);
		File newFile = new File(newf);
		FileDiff diff = new FileDiff(oldFile, newFile);
		try {
			diff.diff();

			List<Delta<String>> insertions = diff.getInsertions();
			for (Delta<String> delta : insertions){
				System.out.println("INSERTION:");
				System.out.println("Revised: line " + delta.getRevised().getPosition()
						+ " length: " + delta.getRevised().getLines().size());
				FileDiff.linesToString(delta.getRevised().getLines());
			}

			List<Delta<String>> changes = diff.getChanges();
			for (Delta<String> delta : changes){
				System.out.println("CHANGE:");
				System.out.println("Original: line " + delta.getOriginal().getPosition()
						+ " length: " + delta.getOriginal().getLines().size());
				FileDiff.linesToString(delta.getOriginal().getLines());
				System.out.println("Revised: line " + delta.getRevised().getPosition()
						+ " length: " + delta.getRevised().getLines().size());
				FileDiff.linesToString(delta.getRevised().getLines());
			}

			List<Delta<String>> deletes = diff.getDeletes();
			for (Delta<String> delta : deletes){
				System.out.println("DELETION:");
				System.out.println("Original: line " + delta.getOriginal().getPosition()
						+ " length: " + delta.getOriginal().getLines().size());
				FileDiff.linesToString(delta.getOriginal().getLines());
			}
		} catch (IOException e) {
			e.printStackTrace();
		}

	}
}

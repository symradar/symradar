package tools.safepatch;

import org.apache.commons.cli.ParseException;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import tools.CommonCommandLineInterface;
/**
 * @author Eric Camellini
 * 
 */
public class SafepatchCmdInterface extends CommonCommandLineInterface {
	
	private String[] filenames;

	public static enum OPTION {
		ENABLE_BV_OPTION("BV"),
		ENABLE_VISUALIZATION("vis"),
		COARSE_GRAINED_DIFF("cgDiff"),
		VERBOSE("verbose"),
		
		OUTPUT_PATH("output");
		private String option;
		
		private OPTION(String option) {
			this.option = option;
		}
		public String getExtension() {
			return option;
		}
	}
	
	public String[] getFilenames()
	{
		return filenames;
	}

	public SafepatchCmdInterface() {
		super();
		this.options.addOption(OPTION.ENABLE_BV_OPTION.option, false, "Use bit vectors when checking condition implications with Z3. When not set, Z3 integer values are used");
		this.options.addOption(OPTION.ENABLE_VISUALIZATION.option, false, "Outputs some useful .dot graphs");
		this.options.addOption(OPTION.COARSE_GRAINED_DIFF.option, false, "Computes and outputs the coarse-grained diff");
		this.options.addOption(OPTION.VERBOSE.option, false, "Outputs runtime information together with the final json result");

		this.options.addOption(OPTION.OUTPUT_PATH.option, true, "Output path for the result file");
	}

	public void parseCommandLine(String[] args) throws ParseException
	{
		if (args.length < 2)
			throw new RuntimeException(
					"Old and new files must be supplied.");

		cmd = parser.parse(options, args);
		filenames = cmd.getArgs();
		
	}

	public void printHelp()
	{
		formater.printHelp("safepatch oldfile newfile", options);
	}
	
	public boolean hasOption(OPTION option){
		return cmd.hasOption(option.option);
	}
	
	public String getOptionValue(OPTION option){
		return cmd.getOptionValue(option.option);
	}
}

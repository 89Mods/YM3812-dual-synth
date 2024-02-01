package tholin.patchUploader;

import javax.swing.JFrame;

public class PatchUploaderFrame {
	public static JFrame frame;
	
	public static void main(String[] args) {
		frame = new JFrame("YM3812 Patch Uploader");
		PatchUploaderPanel panel = new PatchUploaderPanel();
		frame.setContentPane(panel);
		frame.setResizable(false);;
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.pack();
		frame.setLocationRelativeTo(null);
		frame.setVisible(true);
	}
}
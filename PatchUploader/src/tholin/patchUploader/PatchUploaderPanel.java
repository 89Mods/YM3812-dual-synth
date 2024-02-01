package tholin.patchUploader;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.FileInputStream;

import javax.sound.midi.*;
import javax.sound.midi.MidiDevice.Info;
import javax.swing.*;
import javax.swing.filechooser.FileNameExtensionFilter;

public class PatchUploaderPanel extends JPanel {
	private static final long serialVersionUID = -7513373620892892747L;
	
	private JComboBox<String> midiDevSelector;
	private JLabel selectedFileLabel;
	private File selectedFile = null;
	private JButton uploadBtn;
	
	public PatchUploaderPanel() {
		super();
		setPreferredSize(new java.awt.Dimension(440, 175));
		setLayout(new GridBagLayout());
		GridBagConstraints c = new GridBagConstraints();
		c.insets = new Insets(10,0,0,0);
		JLabel l1 = new JLabel("MIDI port for module:");
		c.gridx = 0;
		c.gridy = 0;
		add(l1, c);
		midiDevSelector = new JComboBox<String>();
		midiDevSelector.setEnabled(false);
		c.gridx = 1;
		c.gridy = 0;
		add(midiDevSelector, c);
		JButton refreshBtn = new JButton("Refresh");
		refreshBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				listAllMIDIDevices();
			}
		});
		c.gridx = 2;
		c.gridy = 0;
		add(refreshBtn, c);
		JFileChooser op2Chooser = new JFileChooser();
		op2Chooser.setCurrentDirectory(new File("."));
		op2Chooser.setFileFilter(new FileNameExtensionFilter("OP2 Files", "op2"));
		JButton selBtn = new JButton("Select op2 file");
		selBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				int res = op2Chooser.showOpenDialog(PatchUploaderFrame.frame);
				if(res == JFileChooser.APPROVE_OPTION) {
					File f = op2Chooser.getSelectedFile();
					if(!f.exists()) {
						JOptionPane.showMessageDialog(PatchUploaderFrame.frame, "Selected file not found", "Error", JOptionPane.ERROR_MESSAGE);
						return;
					}
					selectedFile = f;
					selectedFileLabel.setText("Selected file: " + f.getName());
				}
			}
		});
		c.gridx = 0;
		c.gridy = 1;
		add(selBtn, c);
		selectedFileLabel = new JLabel("Selected file: ---");
		c.gridx = 1;
		c.gridy = 1;
		add(selectedFileLabel, c);
		uploadBtn = new JButton("UPLOAD!");
		uploadBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				try {
					uploadPatches();
				}catch(Exception e) {
					JOptionPane.showMessageDialog(PatchUploaderFrame.frame, "Oops, something went wrong there. " + e.getLocalizedMessage(), "Error", JOptionPane.ERROR_MESSAGE);
				}finally {
					uploadBtn.setEnabled(true);
				}
			}
		});
		c.gridx = 0;
		c.gridy = 2;
		c.gridwidth = 3;
		add(uploadBtn, c);
		listAllMIDIDevices();
	}
	
	private void uploadPatches() throws Exception {
		uploadBtn.setEnabled(false);
		if(selectedFile == null) {
			JOptionPane.showMessageDialog(PatchUploaderFrame.frame, "No patch file has been selected yet", "Error", JOptionPane.ERROR_MESSAGE);
			return;
		}
		if(!selectedFile.exists()) {
			JOptionPane.showMessageDialog(PatchUploaderFrame.frame, "Selected file not found", "Error", JOptionPane.ERROR_MESSAGE);
			return;
		}
		String selectedMidiDevName = midiDevSelector.getSelectedItem().toString();
		Info[] midiInfos = MidiSystem.getMidiDeviceInfo();
		Info found = null;
		for(int i = 0; i < midiInfos.length; i++) {
			if(midiInfos[i].getName().equals(selectedMidiDevName)) {
				found = midiInfos[i];
				break;
			}
		}
		if(found == null) {
			JOptionPane.showMessageDialog(PatchUploaderFrame.frame, "The selected MIDI device could not be found (try refreshing the list)", "Error", JOptionPane.ERROR_MESSAGE);
			return;
		}
		MidiDevice dev = MidiSystem.getMidiDevice(found);
		Receiver midiReceiver = dev.getReceiver();
		dev.open();
		byte[] ceData = new byte[5];
		ceData[0] = (byte) 0xF0;
		ceData[1] = 0b01111110;
		ceData[2] = 0b01010110;
		ceData[3] = 0b01110011;
		ceData[4] = (byte) 0b11110111;
		SysexMessage ceMsg = new SysexMessage();
		ceMsg.setMessage(ceData, ceData.length);
		midiReceiver.send(ceMsg, -1);
		Thread.sleep(200);
		
		FileInputStream fis = new FileInputStream(selectedFile);
		fis.skip(8);
		final int instrumentCount = 175;
		for(int k = 0; k < instrumentCount; k += 4) {
			byte[] data = new byte[4*24+5];
			data[0] = (byte) 0xF0;
			data[1] = (byte) (k == 0 ? 0b01111110 : 0);
			data[2] = (byte) (k == 0 ? 0b01010110 : 0);
			data[3] = (byte) (k == 0 ? 0b01111100 : 0);
			data[data.length - 1] = (byte) 0b11110111;
			for(int i = 0; i < 4; i++) {
				fis.skip(3);
				int fixed_note = fis.read();
				
				int tvenvfreq1 = fis.read();
				int atck_dec1 = fis.read();
				int sus_rel1 = fis.read();
				int waveform1 = fis.read();
				int level_scale1 = fis.read() >> 6;
				int level1 = fis.read();
				int feedback_algo = fis.read();
				
				int tvenvfreq2 = fis.read();
				int atck_dec2 = fis.read();
				int sus_rel2 = fis.read();
				int waveform2 = fis.read();
				int level_scale2 = fis.read() >> 6;
				int level2 = fis.read();
				fis.skip(3);
				fis.skip(16);
				
				int ptr = i * 24 + 4;
				data[ptr++] = (byte) (tvenvfreq1 & 0x7F);
				data[ptr++] = (byte) ((tvenvfreq1 >> 7) & 1);
				data[ptr++] = (byte) (level_scale1 & 0x7F);
				data[ptr++] = (byte) ((level_scale1 >> 7) & 1);
				data[ptr++] = (byte) (level1 & 0x7F);
				data[ptr++] = (byte) ((level1 >> 7) & 1);
				data[ptr++] = (byte) (atck_dec1 & 0x7F);
				data[ptr++] = (byte) ((atck_dec1 >> 7) & 1);
				data[ptr++] = (byte) (sus_rel1 & 0x7F);
				data[ptr++] = (byte) ((sus_rel1 >> 7) & 1);
				data[ptr++] = (byte) (waveform1);
				
				data[ptr++] = (byte) (tvenvfreq2 & 0x7F);
				data[ptr++] = (byte) ((tvenvfreq2 >> 7) & 1);
				data[ptr++] = (byte) (level_scale2 & 0x7F);
				data[ptr++] = (byte) ((level_scale2 >> 7) & 1);
				data[ptr++] = (byte) (level2 & 0x7F);
				data[ptr++] = (byte) ((level2 >> 7) & 1);
				data[ptr++] = (byte) (atck_dec2 & 0x7F);
				data[ptr++] = (byte) ((atck_dec2 >> 7) & 1);
				data[ptr++] = (byte) (sus_rel2 & 0x7F);
				data[ptr++] = (byte) ((sus_rel2 >> 7) & 1);
				data[ptr++] = (byte) (waveform2);
				
				data[ptr++] = (byte) (feedback_algo);
				data[ptr++] = (byte) (fixed_note);
			}
			SysexMessage msg = new SysexMessage();
			msg.setMessage(data, data.length);
			midiReceiver.send(msg, -1);	
			Thread.sleep(40);
		}
		fis.close();
		dev.close();
	}
	
	private void listAllMIDIDevices() {
		Info[] midiInfos = MidiSystem.getMidiDeviceInfo();
		String[] names = new String[midiInfos.length];
		for(int i = 0; i < midiInfos.length; i++) {
			Info info = midiInfos[i];
			names[i] = info.getName();
		}
		int selIdx = midiDevSelector.getSelectedIndex();
		if(selIdx < 0) selIdx = 0;
		midiDevSelector.removeAllItems();
		for(int i = 0; i < names.length; i++) midiDevSelector.addItem(names[i]);
		midiDevSelector.setSelectedIndex(Math.min(selIdx, names.length - 1));
		midiDevSelector.setEnabled(true);
	}
}
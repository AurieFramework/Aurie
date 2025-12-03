namespace AurieInstaller
{
    partial class Form_VersionPicker
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            bt_ConfirmVerChoice = new Button();
            cbArVersion = new ComboBox();
            lb_AurieVer = new Label();
            lb_AsVer = new Label();
            cbAsVersion = new ComboBox();
            label1 = new Label();
            cbYkVersion = new ComboBox();
            SuspendLayout();
            // 
            // bt_ConfirmVerChoice
            // 
            bt_ConfirmVerChoice.Location = new Point(12, 197);
            bt_ConfirmVerChoice.Name = "bt_ConfirmVerChoice";
            bt_ConfirmVerChoice.Size = new Size(241, 68);
            bt_ConfirmVerChoice.TabIndex = 0;
            bt_ConfirmVerChoice.Text = "Confirm Version";
            bt_ConfirmVerChoice.UseVisualStyleBackColor = true;
            bt_ConfirmVerChoice.Click += bt_ConfirmVerChoice_Click;
            // 
            // cbAurieVersion
            // 
            cbArVersion.DropDownStyle = ComboBoxStyle.DropDownList;
            cbArVersion.FormattingEnabled = true;
            cbArVersion.Location = new Point(12, 33);
            cbArVersion.Name = "cbAurieVersion";
            cbArVersion.Size = new Size(241, 29);
            cbArVersion.TabIndex = 1;
            cbArVersion.SelectedIndexChanged += cbAurieVersion_SelectedIndexChanged;
            // 
            // lb_AurieVer
            // 
            lb_AurieVer.AutoSize = true;
            lb_AurieVer.Location = new Point(12, 9);
            lb_AurieVer.Name = "lb_AurieVer";
            lb_AurieVer.Size = new Size(135, 21);
            lb_AurieVer.TabIndex = 2;
            lb_AurieVer.Text = "AurieCore version";
            // 
            // lb_AsVer
            // 
            lb_AsVer.AutoSize = true;
            lb_AsVer.Location = new Point(12, 65);
            lb_AsVer.Name = "lb_AsVer";
            lb_AsVer.Size = new Size(143, 21);
            lb_AsVer.TabIndex = 3;
            lb_AsVer.Text = "AurieSharp version";
            // 
            // cbAsVersion
            // 
            cbAsVersion.DropDownStyle = ComboBoxStyle.DropDownList;
            cbAsVersion.FormattingEnabled = true;
            cbAsVersion.Location = new Point(12, 89);
            cbAsVersion.Name = "cbAsVersion";
            cbAsVersion.Size = new Size(241, 29);
            cbAsVersion.TabIndex = 4;
            cbAsVersion.SelectedIndexChanged += cbAsVersion_SelectedIndexChanged;
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new Point(12, 121);
            label1.Name = "label1";
            label1.Size = new Size(128, 21);
            label1.TabIndex = 5;
            label1.Text = "YYToolkit version";
            // 
            // cbYkVersion
            // 
            cbYkVersion.DropDownStyle = ComboBoxStyle.DropDownList;
            cbYkVersion.FormattingEnabled = true;
            cbYkVersion.Location = new Point(12, 145);
            cbYkVersion.Name = "cbYkVersion";
            cbYkVersion.Size = new Size(241, 29);
            cbYkVersion.TabIndex = 6;
            cbYkVersion.SelectedIndexChanged += cbYkVersion_SelectedIndexChanged;
            // 
            // Form_VersionPicker
            // 
            AutoScaleDimensions = new SizeF(9F, 21F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(265, 283);
            Controls.Add(cbYkVersion);
            Controls.Add(label1);
            Controls.Add(cbAsVersion);
            Controls.Add(lb_AsVer);
            Controls.Add(lb_AurieVer);
            Controls.Add(cbArVersion);
            Controls.Add(bt_ConfirmVerChoice);
            Font = new Font("Segoe UI", 12F);
            FormBorderStyle = FormBorderStyle.FixedSingle;
            Margin = new Padding(4);
            Name = "Form_VersionPicker";
            Text = "Select a version";
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion

        private Button bt_ConfirmVerChoice;
        private ComboBox cbArVersion;
        private Label lb_AurieVer;
        private Label lb_AsVer;
        private ComboBox cbAsVersion;
        private Label label1;
        private ComboBox cbYkVersion;
    }
}
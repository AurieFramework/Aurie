namespace AurieInstaller
{
    partial class Form_InstallIntro
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
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
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            bt_PickGame = new Button();
            lb_MainText = new Label();
            label1 = new Label();
            cb_AdvancedMode = new CheckBox();
            cb_InstallAs = new CheckBox();
            SuspendLayout();
            // 
            // bt_PickGame
            // 
            bt_PickGame.Location = new Point(13, 123);
            bt_PickGame.Margin = new Padding(4);
            bt_PickGame.Name = "bt_PickGame";
            bt_PickGame.Size = new Size(302, 61);
            bt_PickGame.TabIndex = 0;
            bt_PickGame.Text = "Find my game!";
            bt_PickGame.UseVisualStyleBackColor = true;
            bt_PickGame.Click += bt_PickGame_Click;
            // 
            // lb_MainText
            // 
            lb_MainText.AutoSize = true;
            lb_MainText.Font = new Font("Segoe UI", 12F, FontStyle.Bold, GraphicsUnit.Point, 238);
            lb_MainText.ForeColor = SystemColors.Highlight;
            lb_MainText.Location = new Point(13, 9);
            lb_MainText.Name = "lb_MainText";
            lb_MainText.Size = new Size(247, 21);
            lb_MainText.TabIndex = 1;
            lb_MainText.Text = "Welcome to the Aurie Installer!\r\n";
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new Point(14, 30);
            label1.Name = "label1";
            label1.Size = new Size(430, 84);
            label1.TabIndex = 2;
            label1.Text = "I don't actually know how to make GUIs, so this is all you get.\r\n\r\nClick the only button available in this application, and\r\nselect your game executable to get started.\r\n";
            // 
            // cb_AdvancedMode
            // 
            cb_AdvancedMode.AutoSize = true;
            cb_AdvancedMode.Checked = true;
            cb_AdvancedMode.CheckState = CheckState.Checked;
            cb_AdvancedMode.Enabled = false;
            cb_AdvancedMode.Location = new Point(322, 123);
            cb_AdvancedMode.Name = "cb_AdvancedMode";
            cb_AdvancedMode.Size = new Size(141, 25);
            cb_AdvancedMode.TabIndex = 3;
            cb_AdvancedMode.Text = "Advanced mode";
            cb_AdvancedMode.UseVisualStyleBackColor = true;
            // 
            // cb_InstallAs
            // 
            cb_InstallAs.AutoSize = true;
            cb_InstallAs.Location = new Point(322, 154);
            cb_InstallAs.Name = "cb_InstallAs";
            cb_InstallAs.Size = new Size(134, 25);
            cb_InstallAs.TabIndex = 4;
            cb_InstallAs.Text = "Allow C# mods";
            cb_InstallAs.UseVisualStyleBackColor = true;
            // 
            // Form_InstallIntro
            // 
            AutoScaleDimensions = new SizeF(9F, 21F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(464, 196);
            Controls.Add(cb_InstallAs);
            Controls.Add(cb_AdvancedMode);
            Controls.Add(label1);
            Controls.Add(lb_MainText);
            Controls.Add(bt_PickGame);
            Font = new Font("Segoe UI", 12F, FontStyle.Regular, GraphicsUnit.Point, 238);
            FormBorderStyle = FormBorderStyle.FixedSingle;
            Margin = new Padding(4);
            MaximizeBox = false;
            MaximumSize = new Size(480, 235);
            MinimumSize = new Size(480, 235);
            Name = "Form_InstallIntro";
            Text = "Aurie Installer v1.0.0";
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion

        private Button bt_PickGame;
        private Label lb_MainText;
        private Label label1;
        private CheckBox cb_AdvancedMode;
        private CheckBox cb_InstallAs;
    }
}

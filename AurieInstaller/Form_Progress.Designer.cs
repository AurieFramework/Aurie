namespace AurieInstaller.Install_Internals
{
    partial class Form_Progress
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
            pb_Status = new ProgressBar();
            lb_Status = new Label();
            SuspendLayout();
            // 
            // pb_Status
            // 
            pb_Status.Location = new Point(12, 36);
            pb_Status.Name = "pb_Status";
            pb_Status.Size = new Size(291, 37);
            pb_Status.TabIndex = 0;
            pb_Status.Value = 10;
            // 
            // lb_Status
            // 
            lb_Status.AutoSize = true;
            lb_Status.Font = new Font("Segoe UI", 12F);
            lb_Status.Location = new Point(12, 9);
            lb_Status.Name = "lb_Status";
            lb_Status.Size = new Size(165, 21);
            lb_Status.TabIndex = 1;
            lb_Status.Text = "Waiting on controller...";
            // 
            // Form_Progress
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(315, 85);
            ControlBox = false;
            Controls.Add(lb_Status);
            Controls.Add(pb_Status);
            FormBorderStyle = FormBorderStyle.FixedSingle;
            MaximizeBox = false;
            MinimizeBox = false;
            Name = "Form_Progress";
            StartPosition = FormStartPosition.CenterScreen;
            Text = "Please wait...";
            TopMost = true;
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion

        public ProgressBar pb_Status;
        public Label lb_Status;
    }
}
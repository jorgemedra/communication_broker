import { Component, HostListener, Inject } from '@angular/core';
import { MatDialog, MAT_DIALOG_DATA, MatDialogRef } from '@angular/material/dialog';

export interface DialogYesNoData {
  text: string;
}

@Component({
  selector: 'app-dialog-yesno',
  templateUrl: './dialog-yesno.component.html',
  styleUrls: ['./dialog-yesno.component.css', '../app.component.css']
})
export class DialogYesnoComponent {
  result: boolean;

  Icon: string;
//   message = 'This is my own message to show from a Modal window.';
//   oklabel = 'Ok';
//   cancellabel = 'Cancel';
//   visible = true;

  constructor(  public dialogRef: MatDialogRef<DialogYesnoComponent>,
               @Inject(MAT_DIALOG_DATA) public data: DialogYesNoData) {
      this.result = true;
      dialogRef.disableClose = true;
      this.Icon = '/assets/img/login.png';
  }

  @HostListener('window:keyup.esc') onKeyUp(){
    this.dialogRef.close();
  }

  onCancel(): void{
    this.dialogRef.close();
    this.result = false;
  }



}





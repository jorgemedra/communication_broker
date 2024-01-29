import { ModelService } from './services/model.service';
import { DbCoreService } from './services/db-core.service';
import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';
import { AppRoutingModule } from './app-routing.module';
import { AppComponent } from './app.component';
import { MatDialogModule, MatDialogRef } from '@angular/material/dialog';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';

import { LoginFormComponent } from './login-form/login-form.component';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { DialogYesnoComponent } from './dialog-yesno/dialog-yesno.component';
import { DialogLogonComponent } from './dialog-logon/dialog-logon.component';
import { Page1Component } from './page1/page1.component';
import { Page2Component } from './page2/page2.component';
import { PageRootComponent } from './page-root/page-root.component';
import { PageNotFoudComponent } from './page-not-foud/page-not-foud.component';

@NgModule({
  declarations: [
    AppComponent,
    LoginFormComponent,
    DialogYesnoComponent,
    DialogLogonComponent,
    Page1Component,
    Page2Component,
    PageRootComponent,
    PageNotFoudComponent
  ],
  imports: [
    BrowserModule,
    AppRoutingModule,
    BrowserAnimationsModule,
    MatDialogModule,
    FormsModule,
    ReactiveFormsModule
  ],
  providers: [
    {
      provide: MatDialogRef,
      useValue: {}
     },
     DbCoreService,
     ModelService
  ],
  bootstrap: [AppComponent]
})
export class AppModule { }

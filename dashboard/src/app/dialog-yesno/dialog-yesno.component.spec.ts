import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DialogYesnoComponent } from './dialog-yesno.component';

describe('DialogYesnoComponent', () => {
  let component: DialogYesnoComponent;
  let fixture: ComponentFixture<DialogYesnoComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ DialogYesnoComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DialogYesnoComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});

import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { PageRootComponent } from './page-root.component';

describe('PageRootComponent', () => {
  let component: PageRootComponent;
  let fixture: ComponentFixture<PageRootComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ PageRootComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(PageRootComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
